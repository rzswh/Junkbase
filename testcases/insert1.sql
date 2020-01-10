create database INSERT_TABLE;
use INSERT_TABLE;
create table Test1 (a int, b char(4), c varchar(100));
insert into Test values (1, "234", "5"), (2, "345", "6");
insert into Test1 values (1, "234");
insert into Test1 values ("1", 234, "5");
insert into Test1 values (1, "234", "5", 6666);
insert into Test1 values (1, "234", "5"), (2, "345", "6");
drop database INSERT_TABLE;
