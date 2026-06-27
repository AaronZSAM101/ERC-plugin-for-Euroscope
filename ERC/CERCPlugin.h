#pragma once
#include "pch.h"
#include <unordered_map>
#include <unordered_set>

class CERCPlugin :
	public EuroScopePlugIn::CPlugIn
{
public:
	CERCPlugin();
	virtual ~CERCPlugin();

	void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan);
	void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);

private:
	bool suppress = false;

	CERCPlugin* pERC;

	std::unordered_map<std::string, std::unordered_set<std::string>> m_SIDSTAR;
	std::unordered_map<std::string, std::string> m_lastSetRoutes;

	void LoadSIDSTARs();
};
