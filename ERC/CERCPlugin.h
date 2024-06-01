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

	void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);

private:
	CERCPlugin* pERC;

	std::unordered_map<std::string, std::unordered_set<std::string>> m_SIDSTAR;

	void LoadSIDSTARs();
};
