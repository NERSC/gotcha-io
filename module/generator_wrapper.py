
from __future__ import print_function
import sys


def generate_wrapper(filename, func_list, modulename, log_file, include_headers, \
				  log_wrap_funcs_list, log_pre_wrap_funcs_list):
        """static gotcha_wrappee_handle_t WRAP_Orig_Func_handle;
static int (*WRAP_Orig_Func) (int param);
static int dissectio_Orig_Func (int param){
	printf("Orig_Func\n");
        WRAP_Orig_Func = gotcha_get_wrappee(WRAP_Orig_Func_handle);
        return WARP_Orig_Func ? (WRAP_Orig_Func(param) ) : 0;
}
"""
	m = modulename.split('_')[0]
	f = open(filename, 'w')
	# write header
	f.write("#include <stdio.h>\n")
	f.write("#include <stdlib.h>\n")
	f.write("#include <unistd.h>\n");
	f.write("#include <time.h>\n");
	f.write("#include \"gotcha/gotcha.h\"\n");
	f.write("#include <pthread.h>\n");
	f.write("#include <sys/types.h>\n");
	f.write("#include <sys/stat.h>\n");
	f.write("#include <fcntl.h>\n\n");
	
	f.write("/* User included headers*/\n")
	for header in include_headers:
		f.write("#include <%s>\n" % header)
	f.write("\n")	
	f.write("#include \"log_init.h\"\n");
	if log_file:
		f.write("#include \"%s.h\"\n"%log_file);
	f.write("\n#define NFUNCS " + str(len(func_list)) + "\n\n")
	f.write("\n")
	f.write("pthread_mutex_t log_mutex;")
	f.write("\n\n")

	
	for function in func_list:
		f.write("static gotcha_wrappee_handle_t " + function.name + "_handle;\n")
                f.write("typedef " + function.ret_type + " (*" + function.name + 
					"_fptr" + ")(" + function.arg_string + ");\n\n")
                f.write(function.ret_type + " __wrap_" + function.name + "(" + \
							function.arg_string + ");\n\n")
	f.write("\n\n");
		
	# write each function wrapper
	for function, log_post_funcs, log_pre_funcs in zip(func_list, log_wrap_funcs_list,\
									log_pre_wrap_funcs_list):
		#write static wrapper
		f.write(function.ret_type + " __wrap_" + function.name + "(" + \
							function.arg_string + "){\n\n")
		if log_pre_funcs:
			f.write("\tpthread_mutex_lock(&log_mutex);\n")
			f.write("\t"+ log_pre_funcs + "(" + function.string_param_names + ");\n")
			f.write("\tpthread_mutex_unlock(&log_mutex);\n\n")	
		f.write("\t" + function.ret_type + " result=" "__real_" + function.name +  "(" + 
					function.string_param_names + ");\n\n")
		
		f.write("\tpthread_mutex_lock(&log_mutex);\n")
		f.write("\t_log_init();\n")
		if log_post_funcs:
			f.write("\t"+ log_post_funcs + "(" + function.string_param_names + ");\n")
		f.write("\tpthread_mutex_unlock(&log_mutex);\n\n")	
                
		f.write("\treturn result;\n")
		f.write("}\n\n\n")
		
		#write shared wrapper
                f.write("static " + function.ret_type + " " + function.wraper + 
						"(" + function.arg_string + "){\n\n")
                
		if log_pre_funcs:
			f.write("\tpthread_mutex_lock(&log_mutex);\n")
			f.write("\t"+ log_pre_funcs + "(" + function.string_param_names + ");\n")
			f.write("\tpthread_mutex_unlock(&log_mutex);\n\n")	
		
		f.write("\t" + function.name + "_fptr" +" " +function.wrapee + 
						" = " + "(" + function.name + "_fptr)" 
						+ "gotcha_get_wrappee(" + function.name 
								+ "_handle);\n\n" )
		f.write("\t" + function.ret_type + " " + "result" + "=" + 
				function.wrapee+"(" + function.string_param_names + ");\n\n")
		
		f.write("\tpthread_mutex_lock(&log_mutex);\n")
		f.write("\t_log_init();\n")
		if log_post_funcs:
                	f.write("\t"+ log_post_funcs +"(" + function.string_param_names + ");\n")
                f.write("\tpthread_mutex_unlock(&log_mutex);\n\n")
		
		f.write("\treturn result;\n")
		f.write("}\n\n\n")
		
		'''
		#write log routine
		f.write("void "+ function.name + "_log(" + function.arg_string + "){\n")
		f.write("\tprintf(\"This is log\");\n")
		f.write("\treturn;")
		f.write("}\n\n")
		'''
		#write common routine
		#f.write("void "+ function.wrapee + "_common(" + function.arg_string + "){\n")
                #f.write("\tt_start = clock();\n")
		#f.write("\t")
                #f.write("}\n\n")

        f.write("\n")

	
	
	# write map struct
	function = func_list[0]
        f.write("static gotcha_binding_t wrap_" + modulename + " [] = {\n")
	f.write("\t{ " + function.name_quote + ", " + function.wraper + 
					", &" + function.name + "_handle" + " }")
	for function in func_list[1:]:
                f.write(",\n\t{ " + function.name_quote + ", " + function.wraper + 
					", &" + function.name + "_handle" + " }")
        f.write("\n};\n\n\n")
        
	# write initializer
        f.write("void init_gotcha_" + modulename + "(){\n")
        f.write("\tgotcha_wrap(wrap_"+ modulename +", NFUNCS, \"wrapper\");\n")
        f.write("}\n\n\n")
        
	#constructor
	f.write("__attribute__((constructor)) void construct_me(){\n")
	f.write("\tinit_gotcha_"+modulename + "();\n")
	f.write("}\n")
	f.close()


