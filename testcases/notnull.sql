create database TEST;
use TEST;
create table Test1 (ind int, a numeric(12, 3) not null, b int, c date);
insert into Test1 values (1, 234.5, 5, "2019-12-31");
insert into Test1 values (2, -9.9999, 4, NULL);
insert into Test1 values (3, 0.0, NULL, "2017-6-8");
insert into Test1 values (4, NULL, 0, "2017-6-8");
select * from Test1;
drop database TEST;