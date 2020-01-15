create database IMPORT;
use IMPORT;
CREATE TABLE PART (

	P_PARTKEY		INT, 
	P_NAME			VARCHAR(55),
	P_MFGR			CHAR(25),
	P_BRAND			CHAR(10),
	P_TYPE			VARCHAR(25),
	P_SIZE			INT,
	P_CONTAINER		CHAR(10),
	P_RETAILPRICE	NUMERIC(10, 10),
	P_COMMENT		VARCHAR(23),
    PRIMARY KEY (P_PARTKEY)
);

COPY part FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/part.tbl" WITH (FORMAT csv, DELIMITER "|");


CREATE TABLE PARTSUPP (
	PS_PARTKEY		INT NOT NULL,
	PS_AVAILQTY		INT,
	PS_SUPPKEY		INT NOT NULL,
	PS_SUPPLYCOST	DECIMAL(20, 2),
	PS_COMMENT		VARCHAR(199),
    PRIMARY KEY (PS_PARTKEY, PS_SUPPKEY)
);

COPY partsupp FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/partsupp.tbl" WITH (FORMAT csv, DELIMITER "|");

CREATE TABLE REGION (
	R_REGIONKEY	INT,
	R_NAME		CHAR(25),
	R_COMMENT	VARCHAR(152),
    PRIMARY KEY(R_REGIONKEY)
);

COPY region FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/region.tbl" WITH (FORMAT csv, DELIMITER "|");

CREATE TABLE NATION (
	N_NATIONKEY		INT,
	N_NAME			CHAR(25),
	N_REGIONKEY		INT NOT NULL,
	N_COMMENT		VARCHAR(152),
    PRIMARY KEY(N_NATIONKEY)
);

COPY nation FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/nation.tbl" WITH (FORMAT csv, DELIMITER "|");



CREATE TABLE SUPPLIER (
	S_SUPPKEY		INT,
	S_NAME			CHAR(25),
	S_ADDRESS		VARCHAR(40),
	S_NATIONKEY		INT NOT NULL,
	S_PHONE			CHAR(15),
	S_ACCTBAL		DECIMAL(20, 2),
	S_COMMENT		VARCHAR(101),
    PRIMARY KEY(S_SUPPKEY)
);

COPY supplier FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/supplier.tbl" WITH (FORMAT csv, DELIMITER "|");





CREATE TABLE CUSTOMER (
	C_CUSTKEY		INT,
	C_NAME			VARCHAR(25),
	C_ADDRESS		VARCHAR(40),
	C_NATIONKEY		INT NOT NULL,
	C_PHONE			CHAR(15),
	C_ACCTBAL		DECIMAL(20, 2),
	C_MKTSEGMENT	CHAR(10),
	C_COMMENT		VARCHAR(117),
    PRIMARY KEY(C_CUSTKEY)
);

COPY customer FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/customer.tbl" WITH (FORMAT csv, DELIMITER "|");








CREATE TABLE ORDERS (
	O_ORDERKEY		INT,
	O_CUSTKEY		INT NOT NULL,
	O_ORDERSTATUS	CHAR(1),
	O_TOTALPRICE	DECIMAL(20, 2),
	O_ORDERDATE		DATE,
	O_ORDERPRIORITY	CHAR(15),
	O_CLERK			CHAR(15),
	O_SHIPPRIORITY	INT,
	O_COMMENT		VARCHAR(79),
    PRIMARY KEY (O_ORDERKEY)
);

COPY orders FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/orders.tbl" WITH (FORMAT csv, DELIMITER "|");





CREATE TABLE LINEITEM (
	L_ORDERKEY		INT NOT NULL, 
	L_PARTKEY		INT NOT NULL, 
	L_SUPPKEY		INT NOT NULL, 
	L_LINENUMBER	INT,
	L_QUANTITY		DECIMAL(20, 2),
	L_EXTENDEDPRICE	DECIMAL(20, 2),
	L_DISCOUNT		DECIMAL(20, 2),
	L_TAX			DECIMAL(20, 2),
	L_RETURNFLAG	CHAR(1),
	L_LINESTATUS	CHAR(1),
	L_SHIPDATE		DATE,
	L_COMMITDATE	DATE,
	L_RECEIPTDATE	DATE,
	L_SHIPINSTRUCT	CHAR(25),
	L_SHIPMODE		CHAR(10),
	L_COMMENT		VARCHAR(44),
    PRIMARY KEY (L_ORDERKEY, L_LINENUMBER)  
);

COPY lineitem FROM "/media/dflasher/Documents/下载/课件/数据库/database_project/testcases/dataset_small_v2/lineitem.tbl" WITH (FORMAT csv, DELIMITER "|");

