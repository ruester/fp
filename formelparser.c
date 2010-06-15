/*
    formelparser - formelparser.c

    Copyright (C) 2010 Matthias Ruester

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>  /* for printf/scanf/fopen/... */
#include <string.h> /* for strcmp/strlen */
#include <limits.h> /* for MAX_INPUT/LINE_MAX */
#include <math.h>   /* for pow */
#include <unistd.h> /* for getopt/optopt/optarg */
#include <ctype.h>  /* isdigit() */


/* #define DEBUG */

/*
 * own includes
 */

#include "node.h"
#include "string.h"
#include "list.h"

/*
 * functions
 */
int isvariable(char c);
int isoperator(char c);
int isbracket(char c);
struct Node *T(struct String *string);
struct Node *S(struct String *string);
struct Node *P(struct String *string);
struct Node *O(struct String *string);
struct Node *K(struct String *string);
struct Node *Num(struct String *string);
struct Node *N(struct String *string);
struct Node *Z(struct String *string);
struct Node *Var(struct String *string);
struct Node *B(struct String *string);
struct Node *create_parse_tree(struct String *string);
long double calculate_parse_tree(struct Node *root);
int count_numerics(int number);
void reduce(struct Node *root);
void print_usage();

/*
 * Grammar:
 * T   -> S | S ? S : S
 * S   -> P | P + P | P - P
 * P   -> O | O * O | O / O | OVar
 * O   -> K | K ^ K
 * K   -> (T) | -(T) | Num | -Num | Var | -Var
 * Num -> N | N 'E' Z | N 'E' - Z
 * N   -> Z | Z . Z
 * Z   -> [0-9]+
 * Var -> B | BZ
 * B   -> [a-z]
 */

/*
 * function checks if a character is a variable
 * 1. argument: a character
 * return value: 0 if character is not a variable
 *               1 if character is a variable
 */
int isvariable(char c)
{
    return(c >= 'a' && c <= 'z');
}

/*
 * function checks if a character is a operator
 * 1. argument: a character
 * return value: 0 if character is not a operator
 *               1 if character is a operator
 */
int isoperator(char c)
{
    return(atoo(c) != ERROR);
}

/*
 * function for conditional expression operators
 * T -> S | S '?' S ':' S
 */
struct Node *T(struct String *tokens)
{
    struct Node *condition, *true, *false, *op;
    
    condition = S(tokens);
    
    if(condition == NULL)
        return(NULL);
    
    while(*(tokens->current) == '?') {
        op = new_ternary_node();
        
        next_char(tokens);
        
        true = S(tokens);
        
        if(*(tokens->current) != ':' || true == NULL) {
            delete_node(op);
            delete_tree(condition);
            return(NULL);
        }
        
        next_char(tokens);
        
        false = S(tokens);
        
        if(false == NULL) {
            delete_node(op);
            delete_tree(condition);
            delete_tree(true);
            return(NULL);
        }
        
        condition = set_3childs(op, condition, true, false);
    }
    
    return(condition);
}

/*
 * function for addition and subtraction signs
 * S -> P | P '+' P | P '-' P
 */
struct Node *S(struct String *tokens)
{
    struct Node *subtree, *subtree2, *op;
    
    subtree = P(tokens);
    
    if(subtree == NULL)
        return(NULL);
    
    while(*(tokens->current) == '+' || (*tokens->current) == '-') {
        op = new_operator_node(*(tokens->current));
        
        next_char(tokens);
        
        subtree2 = P(tokens);
        
        if(subtree2 == NULL) {
            delete_node(op);
            delete_tree(subtree);
            return(NULL);
        }
        
        subtree = set_childs(op, subtree, subtree2);
    }
    
    return(subtree);
}

/*
 * function for multiplication and division signs
 * P -> O | O '*' O | O '/' O | OVar
 */
struct Node *P(struct String *tokens)
{
    struct Node *subtree, *subtree2, *op;
    
    subtree = O(tokens);
    
    if(subtree == NULL)
        return(NULL);
    
    while(isvariable(*(tokens->current))) {
        subtree2 = Var(tokens);
        
        if(subtree2 == NULL) {
            delete_node(subtree);
            return(NULL);
        }
        
        op = new_operator_node('*');
        
        subtree = set_childs(op, subtree, subtree2);
    }
    
    while(*(tokens->current) == '*' || *(tokens->current) == '/') {
        op = new_operator_node(*(tokens->current));
        
        next_char(tokens);
        
        subtree2 = O(tokens);
        
        if(subtree2 == NULL) {
            delete_node(op);
            delete_tree(subtree);
            return(NULL);
        }
        
        subtree = set_childs(op, subtree, subtree2);
    }
    
    return(subtree);
}

/*
 * function for exponentiations
 * O -> K | K '^' K
 */
struct Node *O(struct String *tokens)
{
    struct Node *subtree, *subtree2, *op;
    
    subtree = K(tokens);
    
    if(subtree == NULL)
        return(NULL);
    
    while(*(tokens->current) == '^') {
        op = new_operator_node('^');
        
        next_char(tokens);
        
        subtree2 = K(tokens);
        
        if(subtree2 == NULL) {
            delete_tree(op);
            delete_tree(subtree);
            return(NULL);
        }
        
        subtree = set_childs(op, subtree, subtree2);
    }
    
    return(subtree);
}

/*
 * function for brackets and signed numbers or variables
 * K -> (T) | -(T) | Num | -Num | Var | -Var
 */
struct Node *K(struct String *string)
{
    struct Node *subtree, *subtree2, *op;
    
    subtree = NULL;
    
    if(*(string->current) == '(') {
        next_char(string);
        
        subtree = T(string);
        
        if(subtree == NULL)
            return(NULL);
        
        if(*(string->current) != ')' || subtree == NULL) {
            delete_tree(subtree);
            return(NULL);
        }
        
        next_char(string); /* skip bracket */
    } else {
        if(*(string->current) == '-') {
            next_char(string);
        
            subtree = K(string);
            
            if(subtree == NULL)
                return(NULL);
            
            op = new_operator_node('*');
            
            subtree2 = new_number_node(-1);
            
            subtree = set_childs(op, subtree2, subtree);
        } else {
            if(isdigit(*(string->current)))
                subtree = Num(string);
            else {
                if(isvariable(*(string->current)))
                    subtree = Var(string);
            }
        }
    }
    
    return(subtree);
}

/*
 * function for numbers (maybe with an "E")
 * Num -> N | N 'E' Z | N 'E' '-' Z
 */
struct Node *Num(struct String *tokens)
{
    struct Node *subtree, *subtree2;
    
    subtree = N(tokens); /* get number */
    
    if(subtree == NULL)
        return(NULL);
    
    if(*(tokens->current) == 'E') {
        next_char(tokens); /* skip E symbol */
        
        if(*(tokens->current) == '-') {
            next_char(tokens); /* skip subtraction sign */
            
            subtree2 = Z(tokens); /* get number */
            
            if(subtree2 == NULL) {
                delete_tree(subtree);
                return(NULL);
            }
            
            subtree2 = set_childs(new_operator_node('*'),
                                  new_number_node(-1), subtree2);
            
            subtree = set_childs(new_operator_node('E'),
                                 subtree, subtree2);
        } else {
            subtree2 = Z(tokens); /* get number */
            
            if(subtree2 == NULL) {
                delete_tree(subtree);
                return(NULL);
            }
            
            subtree = set_childs(new_operator_node('E'),
                                 subtree, subtree2);
        }
    }
    
    return(subtree);
}

/*
 * function for numbers
 * N -> Z | Z '.' Z
 */
struct Node *N(struct String *tokens)
{
    struct Node *subtree;
    long double nr;
    int digits;
    
    subtree = Z(tokens);
    
    if(subtree == NULL)
        return(NULL);
    
    if(*(tokens->current) == '.' || *(tokens->current) == ',') {
        next_char(tokens); /* skip point */
        
        nr = subtree->data.value;
        digits = 0;
        
        while(isdigit(*(tokens->current))) {
            digits++;
            
            nr += (*(tokens->current) - '0') / pow(10.0, digits);
            
            next_char(tokens);
        }
        
        subtree->data.value = nr;
    }
    
    return(subtree);
}

/*
 * function for digits
 * Z -> [0-9]+
 */
struct Node *Z(struct String *string)
{
    struct Node *subtree;
    int digits;
    long double number;
    
    subtree = NULL;
    digits = 0;
    number = 0.0;
    
    while(isdigit(*(string->current))) {
        digits = 1;
        
        number = number * 10.0 + (*(string->current) - '0');
        
        next_char(string);
    }
    
    if(!digits)
        return(NULL);
    
    subtree = new_number_node(number);
    
    return(subtree);
}

/*
 * function for variables
 * Var -> B | BZ
 */
struct Node *Var(struct String *string)
{
    struct Node *subtree, *subtree2, *op;
    
    subtree = B(string);
        
    if(subtree == NULL)
        return(NULL);
    
    if(isdigit(*(string->current))) {
        subtree2 = Z(string);
        
        if(subtree2 == NULL) {
            delete_tree(subtree);
            return(NULL);
        }
        
        op = new_operator_node('^');
        
        subtree = set_childs(op, subtree, subtree2);
    }
    
    return(subtree);
}

/*
 * function for characters
 * B -> [a-z]
 */
struct Node *B(struct String *string)
{
    struct Node *subtree;
    
    subtree = NULL;
    
    if(isvariable(*(string->current))) {
        subtree = new_variable_node(*(string->current));
        
        next_char(string);
    }
    
    return(subtree);
}

/*
 * function creates a parse tree from a string
 * 1. argument: tokens of a string
 * return value: the parse tree
 */
struct Node *create_parse_tree(struct String *string)
{
    struct Node *parse_tree;
    
    string->current = string->str;
    
    parse_tree = T(string);
    
    if(*(string->current) != '\0') {

#ifdef DEBUG
        print_tree(parse_tree);
#endif

        printf("syntax error at position %d near '%c'\n", string->i, *(string->current));
        delete_tree(parse_tree);
        return(NULL);
    } else {
        if(*(string->current) == '\0' && parse_tree == NULL)
            printf("syntax error at end of term\n");
    }
    
    return(parse_tree);
}

/*
 * function counts the numerics of a number
 * 1. argument: a number
 * return value: number of numerics
 */
int count_numerics(int number)
{
    int count;
    
    for(count = 0; number > 0; count++)
        number /= 10;
    
    return(count);
}

/*
 * funtion calculates the value of a parse tree
 * 1. argument: pointer of the parse tree
 * return value: the value of the parse tree
 */
long double calculate_parse_tree(struct Node *root)
{
    long double left, right;
    
    switch(root->type) {
        case NUMBER:
            return(root->data.value);
        break;
        
        case OPERATOR:
            left = calculate_parse_tree(root->data.op.left);
            right = calculate_parse_tree(root->data.op.right);
            
            switch(root->data.op.operator) {
                case ADD: return(left + right);
                break;
                
                case MINUS: return(left - right);
                break;
                
                case MULTIPLY: return(left * right);
                break;
                
                case DIVIDE: return(left / right);
                break;
                
                case POWER: return(pow(left, right));
                break;
                
                case E_SYMBOL: return(left * pow(10.0, right));
                break;
            }
        break;
        
        case CONDITIONAL:
            if(calculate_parse_tree(root->data.con.condition))
                return(calculate_parse_tree(root->data.con.true));
            else
                return(calculate_parse_tree(root->data.con.false));
        break;
    }
    
    return(0);
}

/*
 * function finds all variables in a tree
 * and save them in the 2nd argument
 * 1. argument: pointer of the tree
 * 2. argument: adress of the pointer of a string
 *              (first call with empty string)
 * return value: none
 */
void find_variables(struct Node *root, char **var)
{
    char *i;
    
    /* check type of node */
    switch(root->type) {
        case CONDITIONAL:
            /* traverse tree */
            find_variables(root->data.con.condition, var);
            find_variables(root->data.con.true, var);
            find_variables(root->data.con.false, var);
        break;
        
        case OPERATOR:
            /* traverse tree */
            find_variables(root->data.op.left, var);
            find_variables(root->data.op.right, var);
        break;
        
        case VARIABLE:
            for(i = *var; *i != '\0'; i++)
                if(*i == root->data.name)
                    return;
            
            if((*var = realloc(*var, (strlen(*var) + 2)
                               * sizeof(char))) == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            
            (*var)[strlen(*var)] = root->data.name;
            
            (*var)[strlen(*var) + 1] = '\0';
        break;
    }
}

/*
 * function searches a tree for variables
 * 1. argument: pointer of the tree
 * return value: 0 if tree has no variables
 *               1 if tree has variables
 */
int has_variables(struct Node *root)
{
    int ret;
    
    ret = 0;
    
    /* check type of node */
    switch(root->type) {
        case CONDITIONAL:
            /* check subtree */
            ret = has_variables(root->data.con.condition);
            
            if(ret)
                break; /* subtree has a variable */
            
            ret = has_variables(root->data.con.true);
            
            if(ret)
                break;
            
            ret = has_variables(root->data.con.false);
        break;
        
        case OPERATOR:
            ret = has_variables(root->data.op.left);
            
            if(ret)
                break;
            
            ret = has_variables(root->data.op.right);
        break;
        
        case VARIABLE:
            /* leaf is a variable */
            return(1);
        break;
    }
    
    return(ret);
}

/*
 * function replaces one specific variable in a tree
 * 1. argument: variable
 * 2. argument: adress of the pointer of the tree
 * 3. argument: value of the variable (as a node)
 * return value: none
 */
void replace(char b, struct Node **root, struct Node *n)
{
    /* check type of node */
    switch((*root)->type) {
        case CONDITIONAL:
            /* traverse tree */
            replace(b, &((*root)->data.con.condition), n);
            replace(b, &((*root)->data.con.true), n);
            replace(b, &((*root)->data.con.false), n);
        break;
        
        case OPERATOR:
            /* traverse tree */
            replace(b, &((*root)->data.op.left), n);
            replace(b, &((*root)->data.op.right), n);
        break;
        
        case VARIABLE:
            /* check variable */
            if((*root)->data.name == b)
                *root = memcpy(*root, n, sizeof(struct Node));
        break;
    }
}

/* 
 * function replaces all variables in a tree by asking the user for values
 * 1. argument: adress of the pointer of the tree
 * return value: none
 */
void replace_variables(struct Node **root)
{
    char *variables, *i;
    char input[MAX_INPUT];
    struct String *string;
    struct Node *value;
    
    while(has_variables(*root)) {
        /* create emtpy string */
        if((variables = calloc(1, sizeof(char))) == NULL) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }
        
        /* find all variables in tree */
        find_variables(*root, &variables);

#ifdef DEBUG
        print_tree(*root);
#endif

        for(i = variables; *i != '\0'; i++) {
            /* ask for value of variable */
            printf("value of variable %c: ", *i);
            scanf("%s", input);
            
            /* create tokens of input */
            string = new_string(input);
            
            /* create parse tree */
            value = create_parse_tree(string);
            
            if(value == NULL)
                i--;
            
            reduce(value);
            
            /* replace variable in tree */
            replace(*i, root, value);
            
            delete_node(value);
            
            /* free memory of tokens */
            delete_string(string);
        }
        
        /* free memory of string */
        free(variables);
    }
}

void reduce(struct Node *root)
{
    struct Node *left, *right, *old,
                *current, *current2,
                *a, *b, *c, *d, *parent;
    struct List *all, *numbers, *operators, *variables;
    struct Element *rem;
    
    all = NULL;
    numbers = NULL;
    operators = NULL;
    
    if(root == NULL)
        return;
    
    sort_tree(root);
    
    switch(root->type) {
        case OPERATOR:
            reduce(root->data.op.left);
            reduce(root->data.op.right);
            
            left = root->data.op.left;
            right = root->data.op.right;
            
            if(left == NULL || right == NULL)
                return;
            
            switch(root->data.op.operator) {
                case ADD:
                    if(right->type == NUMBER && left->type == NUMBER) {
                        root->type = NUMBER;
                        root->data.value = left->data.value + right->data.value;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(left->type == NUMBER && left->data.value == 0.0) {
                        memcpy(root, right, sizeof(struct Node));
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 0.0) {
                        memcpy(root, left, sizeof(struct Node));
                        delete_node(right);
                        delete_node(left);
                        break;
                    }
                    
                    if(cmp_nodes(left, right)) {
                        root->data.op.operator = MULTIPLY;
                        left->type = NUMBER;
                        left->data.value = 2.0;
                        break;
                    }
                    
                    if(cmp_trees(left, right)) {
                        root->data.op.operator = MULTIPLY;
                        delete_tree(left);
                        root->data.op.left = new_number_node(2.0);
                        break;
                    }
                    
                    all = get_operands(root, ADD);
                    
                    /*
                    printf("all:\n");
                    print_list(all);
                    */
                    
                    if(all == NULL)
                        break;
                    
                    numbers = new_list();
                    operators = new_list();
                    variables = new_list();
                    
                    while(all->current != NULL) {
                        current = all->current->node;
                        
                        if(current->type == OPERATOR)
                            add_node(operators, current);
                        else if(current->type == NUMBER)
                            add_node(numbers, current);
                        else if(current->type == VARIABLE)
                            add_node(variables, current);
                        
                        next_element(all);
                    }
                    
                    rewind_list(operators);
                    
                    /* case: a*4+a*7 -> a*11 */
                    while(operators->current != NULL) {    
                        current = operators->current->node;
                        
                        rem = operators->current;
                        
                        if(current->data.op.operator == MULTIPLY) {
                            next_element(operators);
                            
                            while(operators->current != NULL) {
                                current2 = operators->current->node;
                                
                                if(current2->data.op.operator == MULTIPLY) {
                                    a = current->data.op.left;
                                    b = current->data.op.right;
                                    c = current2->data.op.left;
                                    d = current2->data.op.right;
                                    
                                    if(a->type == OPERATOR || b->type == OPERATOR
                                       || c->type == OPERATOR || d->type == OPERATOR) {
                                        next_element(operators);
                                        continue;
                                    }
                                    
                                    /* 4 cases: */
                                    if(a->type == VARIABLE && b->type == NUMBER
                                       && c->type == VARIABLE && d->type == NUMBER)
                                        b->data.value += d->data.value;
                                    else if(a->type == VARIABLE && b->type == NUMBER
                                            && c->type == NUMBER && d->type == VARIABLE)
                                        b->data.value += c->data.value;
                                    else if(a->type == NUMBER && b->type == VARIABLE
                                            && c->type == VARIABLE && d->type == NUMBER)
                                        a->data.value += d->data.value;
                                    else if(a->type == NUMBER && b->type == VARIABLE
                                            && c->type == NUMBER && d->type == VARIABLE)
                                        a->data.value += c->data.value;
                                    else {
                                        next_element(operators);
                                        continue;
                                    }
                                    
                                    parent = get_parent(root, current2);
                                    
                                    old = parent->data.op.left;
                                    memcpy(parent, parent->data.op.left, sizeof(struct Node));
                                    delete_node(old);
                                    
                                    delete_tree(current2);
                                }
                                
                                next_element(operators);
                            }
                        }
                        
                        operators->current = rem;
                        next_element(operators);
                    }
                    
                    rewind_list(operators);
                    
                    /* case: 2*a+a -> 3*a */
                    while(operators->current != NULL) {
                        current = operators->current->node;
                        
                        if(current->data.op.operator == MULTIPLY) {
                            rewind_list(variables);
                            
                            a = current->data.op.left;
                            b = current->data.op.right;
                            
                            while(variables->current != NULL) {
                                c = variables->current->node;
                                
                                /* 2 cases */
                                if(a->type == VARIABLE && b->type == NUMBER
                                   && a->data.name == c->data.name)
                                    b->data.value += 1.0;
                                else if(a->type == NUMBER && b->type == VARIABLE
                                        && b->data.name == c->data.name)
                                    a->data.value += 1.0;
                                else {
                                    next_element(variables);
                                    continue;
                                }
                                
                                parent = get_parent(root, c);
                                
                                old = parent->data.op.left;
                                memcpy(parent, parent->data.op.left, sizeof(struct Node));
                                delete_node(old);
                                delete_node(c);
                                
                                next_element(variables);
                            }
                        }
                        
                        next_element(operators);
                    }
                    
                    delete_list_without_nodes(operators);
                    delete_list_without_nodes(numbers);
                    delete_list_without_nodes(variables);
                    delete_list_without_nodes(all);
                break;
                
                case MINUS:
                    if(left->type == NUMBER && right->type == NUMBER) {
                        memcpy(root, left, sizeof(struct Node));
                        root->data.value = left->data.value - right->data.value;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(cmp_nodes(left, right)) {
                        root->type = NUMBER;
                        root->data.value = 0.0;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 0.0) {
                        memcpy(root, left, sizeof(struct Node));
                        delete_node(right);
                        delete_node(left);
                        break;
                    }
                    
                    if(cmp_trees(left, right)) {
                        root->type = NUMBER;
                        root->data.value = 0.0;
                        delete_tree(left);
                        delete_tree(right);
                        break;
                    }
                    
                    all = get_operands(root, MINUS);
                    
                    if(all == NULL)
                        break;
                    
                    delete_list_without_nodes(all);
                break;
                
                case MULTIPLY:
                    if(left->type == NUMBER && right->type == NUMBER) {
                        memcpy(root, left, sizeof(struct Node));
                        root->data.value = left->data.value * right->data.value;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if((left->type == NUMBER && left->data.value == 0.0)
                           || (right->type == NUMBER && right->data.value == 0.0)) {
                        root->type = NUMBER;
                        root->data.value = 0.0;
                        delete_tree(left);
                        delete_tree(right);
                        break;
                    }
                    
                    if(left->type == NUMBER && left->data.value == 1.0) {
                        memcpy(root, right, sizeof(struct Node));
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 1.0) {
                        memcpy(root, left, sizeof(struct Node));
                        delete_node(right);
                        delete_node(left);
                        break;
                    }
                    
                    if(cmp_nodes(left, right)) {
                        root->data.op.operator = POWER;
                        right->type = NUMBER;
                        right->data.value = 2.0;
                        break;
                    }
                    
                    all = get_operands(root, MULTIPLY);
                    
                    if(all == NULL)
                        break;
                    
                    delete_list_without_nodes(all);
                break;
                
                case DIVIDE:
                    if(left->type == NUMBER && right->type == NUMBER) {
                        memcpy(root, left, sizeof(struct Node));
                        root->data.value = left->data.value / right->data.value;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(left->type == NUMBER && left->data.value == 0.0) {
                        root->type = NUMBER;
                        root->data.value = 0.0;
                        delete_tree(left);
                        delete_tree(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 0.0) {
                        root->type = NUMBER;
                        root->data.value = 1.0/0.0;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(cmp_nodes(left, right)) {
                        root->type = NUMBER;
                        root->data.value = 1.0;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 1.0) {
                        memcpy(root, left, sizeof(struct Node));
                        delete_node(right);
                        delete_node(left);
                        break;
                    }
                    
                    if(cmp_trees(left, right)) {
                        root->type = NUMBER;
                        root->data.value = 1.0;
                        delete_tree(left);
                        delete_tree(right);
                        break;
                    }
                    
                    all = get_operands(root, DIVIDE);
                    
                    if(all == NULL)
                        break;
                    
                    delete_list_without_nodes(all);
                break;
                
                case POWER:
                    if(left->type == NUMBER && right->type == NUMBER) {
                        memcpy(root, left, sizeof(struct Node));
                        root->data.value = pow(left->data.value, right->data.value);
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(left->type == NUMBER && left->data.value == 0.0) {
                        root->type = NUMBER;
                        root->data.value = 0.0;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 0.0) {
                        root->type = NUMBER;
                        root->data.value = 1.0;
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(right->type == NUMBER && right->data.value == 1.0) {
                        memcpy(root, left, sizeof(struct Node));
                        delete_node(left);
                        delete_node(right);
                        break;
                    }
                    
                    if(left->type == NUMBER && left->data.value == 1.0) {
                        memcpy(root, left, sizeof(struct Node));
                        delete_node(right);
                        delete_node(left);
                        break;
                    }
                    
                    all = get_operands(root, POWER);
                    
                    if(all == NULL)
                        break;
                    
                    delete_list_without_nodes(all);
                break;
                
                case E_SYMBOL:
                    if(left->type == NUMBER && right->type == NUMBER) {
                        root->type = NUMBER;
                        root->data.value = left->data.value * pow(10.0, right->data.value);
                        break;
                    }
                break;
            }
        break;
        
        case CONDITIONAL:
            old = root->data.con.condition;
            left = root->data.con.true;
            right = root->data.con.false;
            
            reduce(root->data.con.condition);
            reduce(root->data.con.true);
            reduce(root->data.con.false);
            
            if(root->data.con.condition->type == NUMBER) {
                if(root->data.con.condition->data.value) {
                    memcpy(root, left, sizeof(struct Node));
                    delete_node(old);
                    delete_node(left);
                    delete_tree(right);
                } else {
                    memcpy(root, right, sizeof(struct Node));
                    delete_node(old);
                    delete_tree(left);
                    delete_node(right);
                }
            }
        break;
    }
}

void print_usage()
{
    printf("usage: formelparser [FORMULA]...\n");
    printf("possible options:\n\t-f [FILE]\n\t-p [PRECISION]\n");
}

int main(int argc, char *argv[])
{
    struct String *string;
    struct Node *parse_tree;
    long double result;
    int i, fromfile;
    short precision;
    char c;
    char read[LINE_MAX];
    char *term, *filename;
    FILE *file;
    
    filename = NULL;
    fromfile = 0;
    term = NULL;
    i = 1;
    precision = 5;
    
    /* no arguments */
    if(argc == 1) {
        print_usage();
        return(1);
    }
    
    /* read arguments */
    while((c = getopt(argc, argv, "f:p:h")) != -1) {
        switch(c) {
            /* get file name */
            case 'f':
                filename = optarg;
                fromfile = 1;
            break;
            
            /* get precision */
            case 'p':
                precision = atoi(optarg);
                
                if(precision < 0)
                    precision = 0;
                
                if(precision > 65)
                    precision = 65;
                
                i = 3;
            break;
            
            /* print help */
            case 'h':
                print_usage();
                return(1);
            break;
            
            /* wrong arguments */
            case '?':
                if(optopt == 'f' || optopt == 'p')
                    printf("option -%c requires an argument\n", optopt);
                else
                    printf("unknown option %c!\n", optopt);
                
                return(1);
            break;
        }
    }
    
    if(fromfile) {
        /* open file */
        if((file = fopen(filename, "r")) == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        
        /* empty file */
        if(fscanf(file, "%s\n", read) == EOF) {
            fclose(file);
            return(0);
        }
    }
    
    do {
        if(fromfile)
            term = read;
        else
            term = argv[i];
        
        /* empty string */
        if(strlen(term) == 0) {
            i++;
            continue;
        }
        
        /* create tokens */
        if((string = new_string(term)) == NULL)
            printf("cannot create tokens for %s\n", term);
        
        /* create parse tree */
        if((parse_tree = create_parse_tree(string)) == NULL) {
            printf("cannot create parse tree for %s\n", term);
            i++;
            free(string->str);
            free(string);
            continue;
        } else {
            /*
            printf("vor Sortierung:\n");
            print_formula(parse_tree, 2);
            
            sort_tree(parse_tree);
            
            printf("nach Sortierung:\n");
            print_formula(parse_tree, 2);
            */
            
            /*
            printf("vorher: ");
            print_formula(parse_tree, precision);
            */
            
            reduce(parse_tree);
            
            /*
            printf("nachher: ");
            print_formula(parse_tree, precision);
            */
            
            /* check tree for variables */
            if(has_variables(parse_tree))
                /* replace variables of tree */
                replace_variables(&parse_tree);

#ifdef DEBUG
            print_tree(parse_tree);
#endif

            reduce(parse_tree);
            
            /* calculate value of parse tree */
            result = calculate_parse_tree(parse_tree);
            
            /* print result */
            printf("%s = %.*Lf\n", term, precision, result);
            
            /* free memory of parse tree */
            delete_tree(parse_tree);
        }
        
        i++;
        
        /* free memory of tokens */
        free(string->str);
        free(string);
    } while(fromfile ? (fscanf(file, "%s\n", read) != EOF) : (argv[i] != NULL));
    
    /* close file */
    if(fromfile)
        fclose(file);
    
    return(0);
}
