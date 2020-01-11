create database TEST2;
use TEST2;
create table Test2 (
    ind int, 
    a numeric(12, 3) not null, 
    b int, 
    c date,
    primary key (ind)
);
insert into Test2 values (1, 234.5, 5, "2019-12-31");
insert into Test2 values (2, -9.9999, 4, NULL);
insert into Test2 values (3, 0.0, NULL, "2017-6-8");
select * from Test2;
insert into Test2 values (1, 365.5, 233, "2019-12-31");
select * from Test2;
alter table Test2 drop primary key;
insert into Test2 values (1, 365.5, 233, "2020-1-1");
select * from Test2;
alter table Test2 add primary key (ind);
alter table Test2 add primary key (ind);
delete from Test2 where c = "2020-1-1";
alter table Test2 add primary key (ind);
insert into Test2 values (1, 365.5, 233, "2020-1-1");
select * from Test2;
drop database TEST2;
