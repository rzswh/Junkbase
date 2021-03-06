#pragma once

const int CREATE_FILE_FAILURE = 101;
const int OPEN_FILE_FAILURE = 102;
const int WRITE_PAGE_FAILURE = 103;

const int OUT_OF_BOUND = 201;
const int ENTRY_NOT_EXIST = 202;
const int EMPTY_RID = 203;
const int MAIN_TABLE_ERROR = 204;
const int INDEX_TABLE_ERROR = 205;

const int TABLE_EXIST = 301;
const int TABLE_NOT_EXIST = 302;
const int NO_ENTRY = 303; // # entries = 0
const int KEY_EXIST = 304;
const int KEY_NOT_EXIST = 305;
const int TOO_MANY_KEYS_COMBINED = 306;
const int REF_NOT_EXIST = 307;
const int ATTR_NOT_EXIST = 308;
const int DUPLICATED_ATTRS = 309;
const int AMBIGUOUS_TABLE = 310;
const int MISSING_TABLE_IN_FROM_CLAUSE = 311;
const int ATTRIBUTE_TYPE_MISMATCH = 312;
const int NOT_IMPLEMENTED_OP = 313;
const int INCOMPLETE_INSERT_DATA = 314;
const int INCOMPATIBLE_TYPE = 315;
const int TOO_MANY_ARGUMENTS = 316;
const int BAD_WHERE_FORMAT = 317;
const int DATE_FORMAT_INVALID = 318;
const int NULL_VALUE_FOR_NOTNULL = 319;
const int INDEX_NOT_EXIST = 320;
const int DUPLICATED_KEY = 321;
const int NULL_VALUE_KEY = 322;
const int UNSUPPORTED_IMPORT_FORMAT = 323;
const int INDEX_EXIST = 324;
const int REF_COL_VAL_MISSING = 325;
const int REF_COL_VAL_CONFLICT = 326;
