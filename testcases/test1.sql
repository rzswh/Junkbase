use import;
select customer.c_name, orders.o_orderstatus, nation.n_nationkey 
from customer,orders,nation 
where customer.c_custkey=orders.o_custkey and customer.c_nationkey=nation.n_nationkey and nation.n_name="china";
