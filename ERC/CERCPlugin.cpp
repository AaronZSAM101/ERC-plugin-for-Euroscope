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

vector<string> Stringsplit(string str, const const char split)
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

	// prepare data to compare
	EuroScopePlugIn::CFlightPlanExtractedRoute extracted_route = FlightPlan.GetExtractedRoute();
	EuroScopePlugIn::CFlightPlanData flightplan_data = FlightPlan.GetFlightPlanData();
	EuroScopePlugIn::CFlightPlanControllerAssignedData controller_assign_data = FlightPlan.GetControllerAssignedData();
	string raw_route = flightplan_data.GetRoute();
	vector<string> splited_raw_route = Stringsplit(raw_route, ' ');
	string sid = flightplan_data.GetSidName();
	string dep_rwy = flightplan_data.GetDepartureRwy();
	string star = flightplan_data.GetStarName();
	string arr_rwy = flightplan_data.GetArrivalRwy();
	string origin = flightplan_data.GetOrigin();
	string destination = flightplan_data.GetDestination();
	int route_count = extracted_route.GetPointsNumber();

	// fliter out all the procedures and runways estimated by ES
	if (dep_rwy != "" && raw_route.find(sid + "/" + dep_rwy) == string::npos && raw_route.find(origin + "/" + dep_rwy) == string::npos) {
		dep_rwy = "";
	}
	if (sid != "" && raw_route.find(sid) == string::npos) {
		sid = "";
	}
	if (arr_rwy != "" && raw_route.find(star + "/" + arr_rwy) == string::npos && raw_route.find(destination + "/" + arr_rwy) == string::npos) {
		arr_rwy = "";
	}
	if (star != "" && raw_route.find(star) == string::npos) {
		star = "";
	}

	// skip if no sid nor star
	if (sid == "" && star == "") {
		suppress = false;
		return;
	}

	// find repeated route
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
			if (point == last_sid_x_point)
			{
				airway_next_to_sid = extracted_route.GetPointAirwayName(i + 1);
			}
		}

		// repeat point in sid
		if (sid != "" && extracted_route.GetPointAirwayName(i) == sid) {
			last_sid_x_point = point;
		}

		// repeat point in star
		if (star != "" && extracted_route.GetPointAirwayName(i + 1) == star) {
			if (first_star_x_point == "") {
				airway_before_star = extracted_route.GetPointAirwayName(i);
				first_star_x_point = point;
			}
			break;
		}
	}

	// find the last airway before STAR begining
	int id_airway_before_star;
	if (airway_before_star == "" || find(splited_raw_route.begin(), splited_raw_route.end(), airway_before_star) == splited_raw_route.end()) {
		id_airway_before_star = -1;
	}
	else {
		id_airway_before_star = distance(splited_raw_route.begin(), find(splited_raw_route.rbegin(), splited_raw_route.rend(), airway_before_star).base()) - 1;
	}

	// generate new route
	bool is_sid_passed = sid == "" && dep_rwy == "";
	string tmp1 = "", tmp2 = "";
	string new_route = "";
	for (int i = 0; i < splited_raw_route.size(); i++)
	{
		// avoid repeat
		if (splited_raw_route[i] == tmp1 || splited_raw_route[i] == tmp2) 
		{
			continue;
		}
		tmp2 = tmp1;
		tmp1 = splited_raw_route[i];

		if (!is_sid_passed && (splited_raw_route[i] == airway_next_to_sid || airway_next_to_sid == ""))
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
					if (dep_rwy == "")
					{
						new_route += sid + " " + last_sid_x_point + " ";
						tmp2 = tmp1;
						tmp1 = last_sid_x_point;
					}
					else
					{
						new_route += sid + "/" + dep_rwy + " " + last_sid_x_point + " ";
						tmp2 = tmp1;
						tmp1 = last_sid_x_point;
					}
				}
			}
			is_sid_passed = true;
		}

		if (i == id_airway_before_star)
		{
			if (arr_rwy == "")
			{
				new_route += splited_raw_route[i] + " " + first_star_x_point + " " + star;
			}
			else
			{
				new_route += splited_raw_route[i] + " " + first_star_x_point + " " + star + "/" + arr_rwy;
			}
			break;
		}

		if (is_sid_passed)
		{
			if (splited_raw_route[i] != sid && splited_raw_route[i] != sid + "/" + dep_rwy && splited_raw_route[i] != origin + "/" + dep_rwy)
			{
				new_route += splited_raw_route[i] + " ";
			}
		}

		if (i == splited_raw_route.size() - 1)
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

