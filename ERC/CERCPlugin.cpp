#include "pch.h"
#include "CERCPlugin.h"
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

CERCPlugin* gpMyPlugin = NULL;

void    __declspec (dllexport)    EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	// create the instance
	*ppPlugInInstance = gpMyPlugin = new CERCPlugin();
}

//---EuroScopePlugInExit-----------------------------------------------

void    __declspec (dllexport)    EuroScopePlugInExit(void)
{
	// delete the instance
	delete gpMyPlugin;
}

void CERCPlugin::LoadSIDSTARs()
{
	// read sector file SID/STAR
	for (auto se = SectorFileElementSelectFirst(EuroScopePlugIn::SECTOR_ELEMENT_SIDS_STARS);
		se.IsValid();
		se = SectorFileElementSelectNext(se, EuroScopePlugIn::SECTOR_ELEMENT_SIDS_STARS))
	{
		m_SIDSTAR[se.GetAirportName()].insert(se.GetName());
	}
}

CERCPlugin::CERCPlugin()
	: EuroScopePlugIn::CPlugIn(
		EuroScopePlugIn::COMPATIBILITY_CODE,
		PLUGIN_NAME,
		PLUGIN_VERSION,
		PLUGIN_DEVELOPER,
		PLUGIN_COPYRIGHT
	)
{
	//LoadSIDSTARs();		// load SIDs and STARs, it's not used current;

	DisplayUserMessage("message", "Exact Route Cliper", string("Version " + MY_PLUGIN_VERSION + " loaded").c_str(), true, false, false, false, false);
}

CERCPlugin::~CERCPlugin()
{
}

std::string& trim(std::string& s) {
	// Trim from the start (left)
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));

	// Trim from the end (right)
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());

	return s;
}

vector<string> Stringsplit(string str, const char split)
{
	istringstream iss(str);
	string token;
	vector<string> rst;
	while (getline(iss, token, split))
	{
		if (token != "") {
			rst.push_back(token);
		}
	}
	return rst;
}

bool HasRouteToken(const vector<string>& route, const string& token)
{
	if (token == "") {
		return false;
	}

	return find(route.begin(), route.end(), token) != route.end();
}

string GetRouteTokenName(const string& token)
{
	return token.substr(0, token.find('/'));
}

bool IsSpeedLevelToken(const string& token)
{
	string name = GetRouteTokenName(token);

	bool is_metric = name.size() == 10 && name[0] == 'K' && name[5] == 'S';
	bool is_imperial = name.size() == 9 && name[0] == 'N' && name[5] == 'F';
	if (!is_metric && !is_imperial) {
		return false;
	}

	for (size_t i = 1; i < name.size(); i++) {
		if (i != 5 && !isdigit(static_cast<unsigned char>(name[i]))) {
			return false;
		}
	}

	return true;
}

bool IsProcedureRouteToken(const string& token)
{
	string name = GetRouteTokenName(token);
	bool has_digit = false;

	if (IsSpeedLevelToken(token)) {
		return false;
	}

	for (char ch : name) {
		if (isdigit(static_cast<unsigned char>(ch))) {
			has_digit = true;
			break;
		}
	}

	return has_digit && !name.empty() && isalpha(static_cast<unsigned char>(name[name.size() - 1]));
}

bool IsSameProcedureToken(const string& token, const string& procedure, const string& runway)
{
	if (procedure == "") {
		return false;
	}

	return token == procedure || (runway != "" && token == procedure + "/" + runway);
}

string GetProcedurePrefixPoint(const string& procedure)
{
	string prefix = "";
	string name = GetRouteTokenName(procedure);

	for (char ch : name) {
		if (isdigit(static_cast<unsigned char>(ch)) || toupper(static_cast<unsigned char>(ch)) == 'X') {
			break;
		}
		prefix += ch;
	}

	return prefix;
}

size_t FindInitialProcedureTokenIndex(const vector<string>& route)
{
	const size_t max_initial_tokens = 3;
	size_t limit = min(route.size(), max_initial_tokens);

	for (size_t i = 0; i < limit; i++) {
		if (IsProcedureRouteToken(route[i])) {
			return i;
		}
	}

	return route.size();
}

string GetProcedureEncodedPoint(const string& procedure)
{
	string prefix = "";
	string suffix = "";
	size_t i = 0;
	string name = GetRouteTokenName(procedure);

	while (i < name.size() && !isdigit(static_cast<unsigned char>(name[i]))) {
		prefix += name[i];
		i++;
	}
	while (i < name.size() && isdigit(static_cast<unsigned char>(name[i]))) {
		i++;
	}
	while (i < name.size()) {
		suffix += name[i];
		i++;
	}

	if (prefix == "" || suffix == "") {
		return "";
	}

	return prefix + suffix;
}

bool IsSidHeadToken(
	const string& token,
	const string& sid,
	const string& dep_rwy,
	const string& sid_transition_exit_point,
	const string& sid_encoded_exit_point,
	const string& last_sid_x_point)
{
	return IsSpeedLevelToken(token) ||
		IsSameProcedureToken(token, sid, dep_rwy) ||
		IsProcedureRouteToken(token) ||
		token == sid_transition_exit_point ||
		token == sid_encoded_exit_point ||
		token == last_sid_x_point;
}

size_t GetSidHeadTokenCount(
	const vector<string>& route,
	const string& sid,
	const string& dep_rwy,
	const string& sid_transition_exit_point,
	const string& sid_encoded_exit_point,
	const string& last_sid_x_point)
{
	if (sid == "" || route.empty()) {
		return 0;
	}

	size_t index = 0;
	if (IsSpeedLevelToken(route[index])) {
		index++;
	}

	if (index >= route.size()) {
		return 0;
	}

	bool matched_sid_head = false;
	string token = route[index];
	if (IsSidHeadToken(token, sid, dep_rwy, sid_transition_exit_point, sid_encoded_exit_point, last_sid_x_point)) {
		index++;
		matched_sid_head = true;
	}

	if (!matched_sid_head) {
		return 0;
	}

	while (index < route.size() &&
		IsSidHeadToken(route[index], sid, dep_rwy, sid_transition_exit_point, sid_encoded_exit_point, last_sid_x_point)) {
		index++;
	}

	return index;
}

void CERCPlugin::OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan)
{
	m_lastSetRoutes.erase(FlightPlan.GetCallsign());
}

void CERCPlugin::OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan)
{
	// exit if not assumed
	if (!FlightPlan.GetTrackingControllerIsMe() || !FlightPlan.IsValid())
	{
		return;
	}

	// state machine to solve repeat callback
	if (suppress) {
		return;
	}
	suppress = true;

	vector<string> route;
	string last_sid_x_point, first_star_x_point;
	string airway_next_to_sid, airway_before_star;
	string sid_transition_exit_point = "";

	// prepare data to compare
	EuroScopePlugIn::CFlightPlanExtractedRoute extracted_route = FlightPlan.GetExtractedRoute();
	EuroScopePlugIn::CFlightPlanData flightplan_data = FlightPlan.GetFlightPlanData();
	EuroScopePlugIn::CFlightPlanControllerAssignedData controller_assign_data = FlightPlan.GetControllerAssignedData();
	string raw_route = flightplan_data.GetRoute();
	string callsign = FlightPlan.GetCallsign();
	auto last_set_route = m_lastSetRoutes.find(callsign);
	if (last_set_route != m_lastSetRoutes.end()) {
		if (last_set_route->second == raw_route) {
			suppress = false;
			return;
		}
		m_lastSetRoutes.erase(last_set_route);
	}
	vector<string> splited_raw_route = Stringsplit(raw_route, ' ');
	string sid = flightplan_data.GetSidName();
	string dep_rwy = flightplan_data.GetDepartureRwy();
	string star = flightplan_data.GetStarName();
	string arr_rwy = flightplan_data.GetArrivalRwy();
	string origin = flightplan_data.GetOrigin();
	string destination = flightplan_data.GetDestination();
	int route_count = extracted_route.GetPointsNumber();
	sid_transition_exit_point = GetProcedurePrefixPoint(sid);

	// fliter out all the procedures and runways estimated by ES
	bool has_sid_token = HasRouteToken(splited_raw_route, sid) || (dep_rwy != "" && HasRouteToken(splited_raw_route, sid + "/" + dep_rwy));
	string sid_encoded_exit_point = GetProcedureEncodedPoint(sid);
	size_t sid_head_token_count = GetSidHeadTokenCount(splited_raw_route, sid, dep_rwy, sid_transition_exit_point, sid_encoded_exit_point, "");
	bool replace_initial_sid = sid != "" && sid_head_token_count > 0;
	if (!replace_initial_sid) {
		size_t initial_sid_index = FindInitialProcedureTokenIndex(splited_raw_route);
		if (initial_sid_index < splited_raw_route.size()) {
			sid_head_token_count = initial_sid_index + 1;
			if (sid_head_token_count < splited_raw_route.size() && (splited_raw_route[sid_head_token_count] == sid_encoded_exit_point || splited_raw_route[sid_head_token_count] == sid_transition_exit_point)) {
				sid_head_token_count++;
			}
			replace_initial_sid = sid != "";
		}
	}
	bool has_star_token = HasRouteToken(splited_raw_route, star) || (arr_rwy != "" && HasRouteToken(splited_raw_route, star + "/" + arr_rwy));
	bool replace_final_star = star != "" && !has_star_token && !splited_raw_route.empty() && IsProcedureRouteToken(splited_raw_route[splited_raw_route.size() - 1]);

	if (dep_rwy != "" && !replace_initial_sid && !HasRouteToken(splited_raw_route, sid + "/" + dep_rwy) && !HasRouteToken(splited_raw_route, origin + "/" + dep_rwy)) {
		dep_rwy = "";
	}
	if (sid != "" && !replace_initial_sid && !HasRouteToken(splited_raw_route, sid) && !HasRouteToken(splited_raw_route, sid + "/" + dep_rwy)) {
		sid = "";
		sid_transition_exit_point = "";
	}
	if (arr_rwy != "" && !replace_final_star && !HasRouteToken(splited_raw_route, star + "/" + arr_rwy) && !HasRouteToken(splited_raw_route, destination + "/" + arr_rwy)) {
		arr_rwy = "";
	}
	if (star != "" && !replace_final_star && !HasRouteToken(splited_raw_route, star) && !HasRouteToken(splited_raw_route, star + "/" + arr_rwy)) {
		star = "";
	}

	// skip if no sid nor star
	if (sid == "" && star == "") {
		suppress = false;
		return;
	}

	// find repeated route
	bool is_sid_seen = false;
	for (int i = 1; i < route_count; i++)
	{
		string point = extracted_route.GetPointName(i);
		// non-repeat point
		if (find(route.begin(), route.end(), point) == route.end()) {
			route.push_back(point);
			if (point == splited_raw_route[splited_raw_route.size() - 1])
			{
				break;
			}
		}
		else
		{
			if (point == last_sid_x_point && i + 1 < route_count)
			{
				airway_next_to_sid = extracted_route.GetPointAirwayName(i + 1);
			}
		}

		// repeat point in sid
		if (sid != "" && extracted_route.GetPointAirwayName(i) == sid) {
			is_sid_seen = true;
			last_sid_x_point = point;
		}
		if (sid_transition_exit_point != "" && is_sid_seen && point == sid_transition_exit_point) {
			last_sid_x_point = point;
			if (i + 1 < route_count) {
				airway_next_to_sid = extracted_route.GetPointAirwayName(i + 1);
			}
		}

		// repeat point in star
		if (star != "" && i + 1 < route_count && extracted_route.GetPointAirwayName(i + 1) == star) {
			if (first_star_x_point == "") {
				airway_before_star = extracted_route.GetPointAirwayName(i);

				string first_star_segment_start = point;
				string first_star_segment_end = extracted_route.GetPointName(i + 1);
				string star_entry_point = GetProcedurePrefixPoint(star);

				if (star_entry_point != "" && find(route.begin(), route.end(), star_entry_point) != route.end()) {
					first_star_x_point = star_entry_point;
				}
				else if (star_entry_point != "" && first_star_segment_start == star_entry_point) {
					first_star_x_point = first_star_segment_start;
				}
				else if (star_entry_point != "" && first_star_segment_end == star_entry_point) {
					first_star_x_point = first_star_segment_end;
				}
				else {
					first_star_x_point = first_star_segment_start;
				}
			}
			break;
		}
	}

	sid_head_token_count = GetSidHeadTokenCount(splited_raw_route, sid, dep_rwy, sid_transition_exit_point, sid_encoded_exit_point, last_sid_x_point);
	replace_initial_sid = sid != "" && sid_head_token_count > 0;

	// find the last airway before STAR begining
	int id_airway_before_star;
	if (airway_before_star == "" || find(splited_raw_route.begin(), splited_raw_route.end(), airway_before_star) == splited_raw_route.end()) {
		id_airway_before_star = -1;
	}
	else {
		id_airway_before_star = distance(splited_raw_route.begin(), find(splited_raw_route.rbegin(), splited_raw_route.rend(), airway_before_star).base()) - 1;
	}

	vector<string> route_to_generate = splited_raw_route;
	if (replace_final_star && !route_to_generate.empty()) {
		route_to_generate.pop_back();
	}
	if (replace_initial_sid && sid_head_token_count > 0 && sid_head_token_count <= route_to_generate.size()) {
		route_to_generate.erase(route_to_generate.begin(), route_to_generate.begin() + sid_head_token_count);
		if (id_airway_before_star >= static_cast<int>(sid_head_token_count)) {
			id_airway_before_star -= static_cast<int>(sid_head_token_count);
		}
		else if (id_airway_before_star >= 0) {
			id_airway_before_star = -1;
		}
	}
	else if (sid != "" && has_sid_token) {
		size_t current_sid_index = route_to_generate.size();
		for (size_t i = 0; i < min(route_to_generate.size(), static_cast<size_t>(3)); i++) {
			if (route_to_generate[i] == sid || route_to_generate[i] == sid + "/" + dep_rwy) {
				current_sid_index = i;
				break;
			}
		}

		size_t stale_sid_index = current_sid_index + 1;
		if (stale_sid_index < route_to_generate.size() && last_sid_x_point != "" && route_to_generate[stale_sid_index] == last_sid_x_point) {
			stale_sid_index++;
		}
		if (stale_sid_index < route_to_generate.size() && IsProcedureRouteToken(route_to_generate[stale_sid_index])) {
			route_to_generate.erase(route_to_generate.begin() + stale_sid_index);
			if (id_airway_before_star > static_cast<int>(stale_sid_index)) {
				id_airway_before_star--;
			}
			else if (id_airway_before_star == static_cast<int>(stale_sid_index)) {
				id_airway_before_star = -1;
			}
		}
	}

	if (replace_initial_sid) {
		string new_route = "";
		if (dep_rwy == "") {
			new_route += sid + " ";
		}
		else {
			new_route += sid + "/" + dep_rwy + " ";
		}

		if (last_sid_x_point != "") {
			new_route += last_sid_x_point + " ";
		}

		for (const auto& token : route_to_generate) {
			new_route += token + " ";
		}

		string trimed_new_route = trim(new_route);
		if (trimed_new_route == raw_route)
		{
			suppress = false;
			return;
		}

		if (flightplan_data.SetRoute(trimed_new_route.c_str()))
		{
			m_lastSetRoutes[callsign] = trimed_new_route;
			if (flightplan_data.IsAmended())
			{
				flightplan_data.AmendFlightPlan();
			}
		}
		else
		{
			DisplayUserMessage("message", "Exact Route Cliper", string("set route fail: " + string(FlightPlan.GetCallsign())).c_str(), true, true, true, true, false);
		}

		suppress = false;
		return;
	}

	// generate new route
	bool is_sid_passed = (sid == "" && dep_rwy == "") || (sid != "" && (HasRouteToken(route_to_generate, sid) || HasRouteToken(route_to_generate, sid + "/" + dep_rwy)));
	string tmp1 = "", tmp2 = "";
	string new_route = "";
	for (size_t i = 0; i < route_to_generate.size(); i++)
	{
		// avoid repeat
		if (route_to_generate[i] == tmp1 || route_to_generate[i] == tmp2)
		{
			continue;
		}
		tmp2 = tmp1;
		tmp1 = route_to_generate[i];

		bool sid_inserted_this_iteration = false;
		bool is_initial_sid_replacement_point = replace_initial_sid && i == 0;
		bool is_sid_insert_point = replace_initial_sid ? is_initial_sid_replacement_point : (route_to_generate[i] == airway_next_to_sid || airway_next_to_sid == "");
		if (!is_sid_passed && is_sid_insert_point)
		{
			if (last_sid_x_point == "")
			{
				if (sid == "")
				{
					if (dep_rwy != "")
					{
						new_route = origin + "/" + dep_rwy + " ";
						tmp2 = tmp1;
						tmp1 = origin + "/" + dep_rwy;
					}
				}
				else
				{
					if (dep_rwy == "")
					{
						new_route += sid + " ";
						tmp2 = tmp1;
						tmp1 = sid;
					}
					else
					{
						new_route += sid + "/" + dep_rwy + " ";
						tmp2 = tmp1;
						tmp1 = sid + "/" + dep_rwy;
					}
				}
			}
			else
			{
				if (sid == "")
				{
					if (dep_rwy == "")
					{
						new_route = last_sid_x_point + " ";
						tmp2 = tmp1;
						tmp1 = last_sid_x_point;
					}
					else
					{
						new_route = origin + "/" + dep_rwy + " " + last_sid_x_point + " ";
						tmp2 = tmp1;
						tmp1 = last_sid_x_point;
					}
				}
				else
				{
					bool append_sid_exit_point = last_sid_x_point != "";
					if (dep_rwy == "")
					{
						new_route += sid + " ";
						if (append_sid_exit_point) {
							new_route += last_sid_x_point + " ";
						}
						tmp2 = tmp1;
						tmp1 = append_sid_exit_point ? last_sid_x_point : sid;
					}
					else
					{
						new_route += sid + "/" + dep_rwy + " ";
						if (append_sid_exit_point) {
							new_route += last_sid_x_point + " ";
						}
						tmp2 = tmp1;
						tmp1 = append_sid_exit_point ? last_sid_x_point : sid + "/" + dep_rwy;
					}
				}
			}
			is_sid_passed = true;
			sid_inserted_this_iteration = true;
		}

		if (id_airway_before_star >= 0 && i == static_cast<size_t>(id_airway_before_star))
		{
			if (arr_rwy == "")
			{
				new_route += route_to_generate[i] + " " + first_star_x_point + " " + star;
			}
			else
			{
				new_route += route_to_generate[i] + " " + first_star_x_point + " " + star + "/" + arr_rwy;
			}
			break;
		}

		if (is_sid_passed)
		{
			if (route_to_generate[i] != sid && route_to_generate[i] != sid + "/" + dep_rwy && route_to_generate[i] != origin + "/" + dep_rwy && !(sid_inserted_this_iteration && route_to_generate[i] == last_sid_x_point))
			{
				new_route += route_to_generate[i] + " ";
			}
		}

		if (i == route_to_generate.size() - 1)
		{
			if (star != "") {
				if (arr_rwy == "")
				{
					if (tmp1 != star && tmp2 != star) {
						new_route += star;
						tmp2 = tmp1;
						tmp1 = star;
					}
				}
				else
				{
					if(tmp1 != star + "/" + arr_rwy && tmp2 != star + "/" + arr_rwy){
						new_route += star + "/" + arr_rwy;
						tmp2 = tmp1;
						tmp1 = star + "/" + arr_rwy;
					}
				}
			}
		}
	}

	// trim the new route
	string trimed_new_route = trim(new_route);

	// if no changes on the route, exit
	if (trimed_new_route == raw_route)
	{
		suppress = false;
		return;
	}

	// set route replace into the flightplan
	if (flightplan_data.SetRoute(trimed_new_route.c_str()))
	{
		m_lastSetRoutes[callsign] = trimed_new_route;
		if (flightplan_data.IsAmended())
		{
			flightplan_data.AmendFlightPlan();
		}
	}
	else
	{
		DisplayUserMessage("message", "Exact Route Cliper", string("set route fail: " + string(FlightPlan.GetCallsign())).c_str(), true, true, true, true, false);
	}

	suppress = false;
}

