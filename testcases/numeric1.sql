create database NUMERIC_TABLE;
use NUMERIC_TABLE;
create table Test1 (ind int, a numeric(12, 3), b int, c date);
insert into Test1 values (1, 234.5, 5, "2019-12-31"), (2, -9.9999, 4, NULL),
 (3, 0.0, NULL, "2017-6-8"),(4, NULL, NULL, NULL);
select * from Test1;
select * from Test1 where a < 3.14;
update Test1 set a = 13333333333.333 where a = -9.9999;
select * from Test1;
delete from Test1 where a < 0.;
select * from Test1;
drop database NUMERIC_TABLE;