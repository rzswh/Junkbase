create database FOREIGN_TEST;
use FOREIGN_TEST;
create table Adding(a int, b int, c char(10));
create table Minus(a char(10), b char(8), c int, d int);
create table Multiply(a int , b char(8), c int);
alter table Minus add constraint Foreign1 foreign key (c, d) references Adding(a, b);
alter table Multiply add constraint Foreign2 foreign key (a, c) references Adding(b, a);
insert into Minus values ("test", "test2", 1, 2);
insert into Multiply values (1, "swh", 4);
insert into Adding values (1, 2, "4");
select * from Adding;
desc Adding;
drop database FOREIGN_TEST;