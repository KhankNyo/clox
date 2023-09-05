import os


fun_name = "fn"


def break_lox():
   test = open("test.lox", "w")
   write_fun(test, 127)
   test.write("print \"OK\";")
   test.close()


def write_fun(file, fun_count, indent_spaces = 0):
    if (fun_count > 0):
        indent(file, indent_spaces)
        file.write("fun "+fun_name+str(fun_count)+"(){\n")

        write_fun(file, fun_count - 1, indent_spaces + 2)

        indent(file, indent_spaces)
        file.write("}\n")


def indent(file, num_spaces):
    file.write(" "*num_spaces)


break_lox()
