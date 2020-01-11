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
select * from part;
drop database IMPORT;