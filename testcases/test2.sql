use import;
create table nationback(
n_nationkey int not null,
n_name char(25) not null,
n_regionkey int not null,
n_comment varchar(152)
);
drop table nationback;
show tables;