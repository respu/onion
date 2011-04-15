/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <stdarg.h>

#include "onion/log.h"
#include "list.h"
#include "block.h"
#include "parser.h"
#include <malloc.h>


int work(const char *infilename, FILE *in, FILE *out);

int main(int argc, char **argv){
	if (argc!=3){
		ONION_ERROR("Usage: otemplate <inputfile> <outputfile>");
		return 1;
	}
	
	FILE *in;
	const char *infilename;
	if (strcmp(argv[1], "-")==0){
		in=stdin;
		infilename="";
	}
	else{
		in=fopen(argv[1], "rt");
		infilename=argv[1];
	}
	
	FILE *out;
	if (strcmp(argv[2], "-")==0)
		out=stdout;
	else
		out=fopen(argv[2], "wt");
	
	if (!in || !out){
		ONION_ERROR("Error opening input or output file");
		return 1;
	}
	
	int error=work(infilename, in,out);
	
	fclose(in);
	fclose(out);
	
	return error;
}


int work(const char *infilename, FILE *in, FILE *out){
	parser_status status;
	memset(&status, 0, sizeof(status));
	status.in=in;
	status.out=out;
	status.mode=TEXT;
	status.functions=list_new((void*)function_free);
	status.function_stack=list_new(NULL);
	status.status=0;
	status.line=0;
	status.rawblock=block_new();
	status.infilename=infilename;
	
	function_new(&status, basename(strdupa(infilename)));
	
	parse_template(&status);
	
	if (status.status){
		ONION_ERROR("Parsing error");
		list_free(status.functions);
		return status.status;
	}
	
	fprintf(out,
"/** Autogenerated by otemplate v. 0.0.0 */\n"
"\n"
"#include <libintl.h>\n"
"#include <string.h>\n\n"
"#include <onion/onion.h>\n"
"#include <onion/dict.h>\n"
"\n"
"typedef struct dict_res_t{\n"
"	onion_dict *dict;\n"
"	onion_response *res;\n"
"}dict_res;\n"
"\n"
"\n");
	
	functions_write_declarations(&status);

	functions_write_main_code(&status);


	functions_write_code(&status);
	
	list_free(status.functions);
	list_free(status.function_stack);
	block_free(status.rawblock);
	
	return status.status;
}
