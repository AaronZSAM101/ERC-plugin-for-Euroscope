# ERC
Exact Route Cliper plugin for Euroscope

![GitHub License](https://img.shields.io/github/license/LeoChen98/ERC-plugin-for-Euroscope)
![GitHub Release](https://img.shields.io/github/v/release/LeoChen98/ERC-plugin-for-Euroscope)
![GitHub Repo stars](https://img.shields.io/github/stars/LeoChen98/ERC-plugin-for-Euroscope)

## How to use
- Download the [latest release](https://github.com/LeoChen98/ERC-plugin-for-Euroscope/releases/latest) file
- Load the ERC.dll into Euroscope

## How it works
- Once the flightplan updated, Euroscope calls the OnFlightPlanFlightPlanDataUpdate to start the process.
- Get SID, STAR, departure runway, arrival runway have been set on the flightplan and clear the expected by Euroscope.
- Find the last waypoint in SID cross with flightplan route and the airway next to it, and the first waypoint in STAR cross with flightplan route and the airway before it.
- Replace the SID and STAR with exactly route into flightplan.

## Developing
If you have any question or any bug to report, please open an issue.
