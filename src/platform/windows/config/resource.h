// -*- coding: utf-8 -*-
// resource.h - Resource definitions for LaprdusTTS configuration GUI

#ifndef LAPRDUS_CONFIG_RESOURCE_H
#define LAPRDUS_CONFIG_RESOURCE_H

// Dialog
#define IDD_CONFIG_DIALOG       100

// Application icon
#define IDI_APP_ICON            101

// Group boxes
#define IDC_GROUP_VOICE         1001
#define IDC_GROUP_PAUSES        1002
#define IDC_GROUP_OPTIONS       1003
#define IDC_GROUP_DICTIONARIES  1004

// Voice controls
#define IDC_VOICE_LABEL         1010
#define IDC_VOICE_COMBO         1011

// Speed controls
#define IDC_SPEED_LABEL         1020
#define IDC_SPEED_SLIDER        1021
#define IDC_SPEED_VALUE         1022
#define IDC_FORCE_SPEED         1023

// Pitch controls
#define IDC_PITCH_LABEL         1030
#define IDC_PITCH_SLIDER        1031
#define IDC_PITCH_VALUE         1032
#define IDC_FORCE_PITCH         1033

// Volume controls
#define IDC_VOLUME_LABEL        1040
#define IDC_VOLUME_SLIDER       1041
#define IDC_VOLUME_VALUE        1042
#define IDC_FORCE_VOLUME        1043

// Pause controls
#define IDC_COMMA_LABEL         1050
#define IDC_COMMA_SLIDER        1051
#define IDC_COMMA_VALUE         1052

#define IDC_SENTENCE_LABEL      1060
#define IDC_SENTENCE_SLIDER     1061
#define IDC_SENTENCE_VALUE      1062

#define IDC_NEWLINE_LABEL       1070
#define IDC_NEWLINE_SLIDER      1071
#define IDC_NEWLINE_VALUE       1072

// Inflection level and acceleration (every voice)
#define IDC_INFLECTION_LABEL    1075
#define IDC_INFLECTION_SLIDER   1076
#define IDC_INFLECTION_VALUE    1077
#define IDC_ACCELERATION_LABEL  1078
#define IDC_ACCELERATION_SLIDER 1079
#define IDC_ACCELERATION_VALUE  1084

// Spelling speed (in the pauses group)
#define IDC_SPELLING_SPEED_LABEL  1073
#define IDC_SPELLING_SPEED_SLIDER 1074
#define IDC_SPELLING_SPEED_VALUE  1085

// Options checkboxes
#define IDC_EMOJI_CHECK         1081
#define IDC_DIGITS_CHECK        1082
#define IDC_SPELLING_SOUNDS_CHECK 1086   // spell letters by their sounds, not names

// Buttons
#define IDC_TEST_BUTTON         1090
#define IDC_APPLY_BUTTON        1091
// IDOK and IDCANCEL are predefined in Windows

// User dictionary controls (in main dialog)
#define IDC_USER_DICT_CHECK     1095
#define IDC_DICT_BUTTON         1096

// Dictionary Management Dialog
#define IDD_DICTIONARY_DIALOG   200
#define IDC_DICT_TYPE_LABEL     2001
#define IDC_DICT_TYPE_COMBO     2002
#define IDC_DICT_LISTVIEW       2003
#define IDC_DICT_ADD            2004
#define IDC_DICT_EDIT           2005
#define IDC_DICT_DUPLICATE      2006
#define IDC_DICT_DELETE         2007
#define IDC_DICT_CLOSE          2008

// Dictionary Entry Editor Dialog
#define IDD_DICT_ENTRY_DIALOG       201
#define IDC_ENTRY_ORIGINAL_LABEL    2101
#define IDC_ENTRY_ORIGINAL_EDIT     2102
#define IDC_ENTRY_REPLACEMENT_LABEL 2103
#define IDC_ENTRY_REPLACEMENT_EDIT  2104
#define IDC_ENTRY_COMMENT_LABEL     2105
#define IDC_ENTRY_COMMENT_EDIT      2106
#define IDC_ENTRY_CASE_SENSITIVE    2107
#define IDC_ENTRY_WHOLE_WORD        2108

// String IDs
#define IDS_APP_TITLE           1100
#define IDS_GROUP_VOICE         1101
#define IDS_VOICE_LABEL         1102
#define IDS_SPEED_LABEL         1103
#define IDS_FORCE_SPEED         1104
#define IDS_PITCH_LABEL         1105
#define IDS_FORCE_PITCH         1106
#define IDS_VOLUME_LABEL        1107
#define IDS_FORCE_VOLUME        1108
#define IDS_GROUP_PAUSES        1109
#define IDS_COMMA_LABEL         1110
#define IDS_SENTENCE_LABEL      1111
#define IDS_NEWLINE_LABEL       1112
#define IDS_GROUP_OPTIONS       1113
#define IDS_EMOJI               1115
#define IDS_DIGITS              1116
#define IDS_TEST                1117
#define IDS_OK                  1118
#define IDS_CANCEL              1119
#define IDS_APPLY               1120
#define IDS_VALUE_SPEED         1121  // "%.1fx"
#define IDS_VALUE_PERCENT       1122  // "%d%%"
#define IDS_VALUE_MS            1123  // "%d ms"
#define IDS_INFLECTION_LABEL    1125
#define IDS_ACCELERATION_LABEL  1126
#define IDS_VALUE_WPM           1127  // "%.1fx (%d WPM)"
#define IDS_SPELLING_SPEED_LABEL 1128
#define IDS_SPELLING_SOUNDS     1129

// Voice names for combo box
#define IDS_VOICE_JOSIP         1130
#define IDS_VOICE_VLADO         1131
#define IDS_VOICE_DETENCE       1132
#define IDS_VOICE_BABA          1133
#define IDS_VOICE_DJEDO         1134
#define IDS_VOICE_ZVONKO        1135
#define IDS_VOICE_STOJAN        1136
#define IDS_VOICE_MIRSAD        1137
#define IDS_VOICE_ORGULJAS      1150
#define IDS_VOICE_KLAPA         1151
#define IDS_VOICE_TRUBAC        1152
#define IDS_VOICE_HARMONIKAS    1153
#define IDS_VOICE_SEVDALIJA     1154
#define IDS_VOICE_SAZLIJA       1155
#define IDS_VOICE_PJEVAC        1156
#define IDS_VOICE_PEVAC         1157
#define IDS_VOICE_SOLIST        1158
#define IDS_VOICE_BECARAC       1159

// Test text
#define IDS_TEST_TEXT           1140

// Dictionary dialog strings
#define IDS_USER_DICT_CHECK         1200
#define IDS_DICT_BUTTON             1201
#define IDS_DICT_DIALOG_TITLE       1202
#define IDS_DICT_TYPE_LABEL         1203
#define IDS_DICT_TYPE_MAIN          1204
#define IDS_DICT_TYPE_SPELLING      1205
#define IDS_DICT_TYPE_EMOJI         1206
#define IDS_DICT_COL_ORIGINAL       1207
#define IDS_DICT_COL_REPLACEMENT    1208
#define IDS_DICT_COL_CASE           1209
#define IDS_DICT_COL_WHOLE_WORD     1210
#define IDS_DICT_COL_COMMENT        1211
#define IDS_DICT_ADD                1212
#define IDS_DICT_EDIT               1213
#define IDS_DICT_DUPLICATE          1214
#define IDS_DICT_DELETE             1215
#define IDS_DICT_CLOSE              1216
#define IDS_ENTRY_DIALOG_TITLE_ADD  1217
#define IDS_ENTRY_DIALOG_TITLE_EDIT 1218
#define IDS_ENTRY_ORIGINAL_LABEL    1219
#define IDS_ENTRY_REPLACEMENT_LABEL 1220
#define IDS_ENTRY_COMMENT_LABEL     1221
#define IDS_ENTRY_CASE_SENSITIVE    1222
#define IDS_ENTRY_WHOLE_WORD        1223
#define IDS_ERROR_FILE_CREATE       1230
#define IDS_ERROR_FILE_WRITE        1231
#define IDS_ERROR_FILE_READ         1232
#define IDS_ERROR_INVALID_ENTRY     1233
#define IDS_YES                     1234
#define IDS_NO                      1235
#define IDS_GROUP_DICTIONARIES      1236

#endif // LAPRDUS_CONFIG_RESOURCE_H
