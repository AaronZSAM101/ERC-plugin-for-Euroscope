// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include "EuroScopePlugIn.h"
#include <string>

using namespace std;

// Version Info Define
#define PLUGIN_NAME      "Exact Route Cliper"
#define PLUGIN_VERSION   "1.0.4.4"
#define PLUGIN_DEVELOPER "Leo Chen"
#define PLUGIN_COPYRIGHT "Copyright (C) 2024 Leo Chen, lisenced under GPL v3."

const string MY_PLUGIN_NAME = "Exact Route Cliper";
const string MY_PLUGIN_VERSION = "1.0.4.4";
const string MY_PLUGIN_DEVELOPER = "Leo Chen";
const string MY_PLUGIN_COPYRIGHT = "Copyright (C) 2024 Leo Chen, lisenced under GPL v3.";

#endif //PCH_H
