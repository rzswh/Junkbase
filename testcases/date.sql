create database DATE_TABLE;
use DATE_TABLE;
create table Test1 (ind int, a char(12), b int, c date);
select * from Test1;
insert into Test1 values (1, "2019-1-2", 5, "2019-12-31"), (2, NULL, 6, "2020-2-2"),
 (3, "2", NULL, "2017-6-8");
select * from Test1;
insert into Test1 values (4, "1", 2, "2100-2-29");
insert into Test1 values (5, "1", 2, "2100.2.29");
insert into Test1 values (6, "1", 2, NULL);
select * from Test1;
update Test1 set c = "1990-12-31" where c = "2019-12-31";
select * from Test1 where c <= "2017-6-8";
drop database DATE_TABLE;