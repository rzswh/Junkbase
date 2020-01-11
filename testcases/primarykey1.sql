create database TEST;
use TEST;
create table Test1 (
    ind int, 
    a numeric(12, 3) not null, 
    b int, 
    c date
);
insert into Test1 values (1, 234.5, 5, "2019-12-31");
insert into Test1 values (2, -9.9999, 4, NULL);
insert into Test1 values (1, 0.0, NULL, "2017-6-8");
alter table Test1 add primary key (ind);
delete from Test1 where b is null;
insert into Test1 values (NULL, 1.0, 2, "2077-8-8");
alter table Test1 add primary key (ind);
delete from Test1 where c > "2050-1-1";
insert into Test1 values (-2, -9.9, 6, NULL);
alter table Test1 add primary key (ind);
select * from Test1;
drop database TEST;
