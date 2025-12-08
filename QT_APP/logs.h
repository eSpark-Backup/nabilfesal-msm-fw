#pragma once

// ==============================
// ANSI color macros for tags
// ==============================

// Foreground colors
#define F_BLACK             "30"
#define F_RED               "31"
#define F_GREEN             "32"
#define F_YELLOW            "33"
#define F_BLUE              "34"
#define F_MAGENTA           "35"
#define F_CYAN              "36"
#define F_GREY              "37"
#define F_WHITE             "97"

// Background colors
#define B_BLACK             "40"
#define B_RED               "41"
#define B_GREEN             "42"
#define B_YELLOW            "43"
#define B_BLUE              "44"
#define B_MAGENTA           "45"
#define B_CYAN              "46"
#define B_GREY              "47"
#define B_WHITE             "107"

// Helper macro to create colored tag
#define COLOR_TAG(fg, bg, text) "\033[1;" fg ";" bg "m" text "\033[0m"

// Predefined tags for MQTT clients
#define TAG_ADMIN   COLOR_TAG(F_BLUE,  B_WHITE, "[ADMIN]")
#define TAG_FACTORY COLOR_TAG(F_GREEN, B_WHITE, "[FACTORY]")

// Optional: quick log macros
#define LOG_DEBUG(tag, msg) qDebug()   << tag << msg
#define LOG_WARN(tag, msg)  qWarning() << tag << msg
#define LOG_INFO(tag, msg)  qInfo()    << tag << msg

#define logDebug    qDebug().noquote()      << this->tag
#define logInfo     qInfo().noquote()       << this->tag
#define logWarn     qWarning().noquote()    << this->tag
#define logError    qCritical().noquote()   << this->tag
