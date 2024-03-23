#include "syntax_tree.h"
extern syntax_tree *parse(const char*);
extern int yydebug;
int main(int argc, char *argv[])
{
    yydebug = 1;
    syntax_tree *tree = NULL;
    const char *input = NULL;

    if (argc == 2) {
        input = argv[1];
    } else if(argc >= 3) {
        printf("usage: %s <cminus_file>\n", argv[0]);
        return 1;
    }

    // Call the syntax analyzer.
    printf("Parsing %s\n", input);
    tree = parse(input);
    print_syntax_tree(stdout, tree);
    del_syntax_tree(tree);
    return 0;
}
