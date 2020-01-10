create database VARCHAR_TABLE;
use VARCHAR_TABLE;
create table Test1 (ind int, a varchar(12), b int, c date);
insert into Test1 values (1, "2019-1-2", 5, "2019-12-31"), (2, NULL, 6, "2020-2-2"),
 (3, "2", NULL, "2017-6-8");
select * from Test1;
update Test1 set a = "abcdefg" where ind = 3;
select * from Test1;
delete from Test1 where ind <= 2;
select * from Test1;
drop database VARCHAR_TABLE;