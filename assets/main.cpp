#include "version.h"
#include <cstdio>

int main(int argc, char **argv) {
    const Version version;
    printf("package %s\n", version.package);
    printf("git sha = %s\n", version.git_sha1);
    printf("git decription = %s\n", version.git_description);
    return 0;
}

