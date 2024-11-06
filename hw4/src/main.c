#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cookbook.h"

int main(int argc, char *argv[]) {
    COOKBOOK *cbp;
    int err = 0;
    char *cookbook = "rsrc/cookbook.ckb";
    FILE *in;

    if((in = fopen(cookbook, "r")) == NULL) {
	fprintf(stderr, "Can't open cookbook '%s': %s\n", cookbook, strerror(errno));
	exit(1);
    }
    cbp = parse_cookbook(in, &err);
    if(err) {
	fprintf(stderr, "Error parsing cookbook '%s'\n", cookbook);
	exit(1);
    }
    unparse_cookbook(cbp, stdout);
    exit(0);
}
