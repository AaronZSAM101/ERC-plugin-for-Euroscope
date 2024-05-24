#pragma once
#include "pch.h"

class CERCPlugin :
	public EuroScopePlugIn::CPlugIn
{
public:
	CERCPlugin();
	virtual ~CERCPlugin();

	void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);

private:
	CERCPlugin* pERC;
};
