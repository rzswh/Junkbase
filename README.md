# Junkbase

Junkbase is a homework project for Introduction to Database Managerment System course in THU. Some interface design is referenced to Stanford CS346 project Redbase documents.

## How to build

Make sure you have `flex` and `bison` installed for lexer and parser generation. For Ubuntu systems, install them by:

```sh
sudo apt install flex bison
```

Build by executing the following command:

```sh
make
```

## Features

- Fixed-length records combined with variant data storage
- B-Plus tree based index
- Five types of data: `int`, `char`, `varchar`, `numeric`, `date`
- Primary key and foreign key supported

(Below is a Chinese version copied from by final report)

- 定长为基础，结合了变长数据的底层记录管理模块；
- 基于B+树的唯一型、可联合索引；
- 数据库的创建、删除、切换；
- 数据表的创建、删除；
- 索引的创建、删除、打开；
- 五种数据类型：`INT`、`CHAR`、`VARCHAR`、`NUMERIC`、`DATE`；每种类型都支持`NULL`，`WHERE`子句可以对`NULL`进行判断；支持字符串字典序比较；
- 插入、删除、更新数据，插入中要求满足数据完整性约束，如主键和外键；
- 通过`WHERE`子句控制选择、更新、删除的范围，支持一定的类型转换；
- `SELECT`语句，简单地暴力枚举，可支持多表连接，`WHERE`子句可以跨表约束；
- 利用索引实现了主键；
- 可以在定义表的时候构造主键、外键，要求后来插入的数据满足约束；
- 支持从外部文件中向数据表中导入数据；
- 向现有表格中添加主键，检查表中已有数据是否满足约束，并更新索引（外键实现存在bug，暂不支持）。

## Known Bugs

When you attempt to add an index to a table where some records already exist, you will get an error which indicates "missing value in the referenced column".
I'm not gonna fix this, since nobody would give me better grades :(
