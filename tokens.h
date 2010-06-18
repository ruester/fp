/*
    formelparser - tokens.h

    Copyright (C) 2010 Matthias Ruester
    Copyright (C) 2010 Max Planck Institut for Molecular Genetics

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

#ifndef TOKENS_H
#define TOKENS_H

struct Tokens {
    char *current;
    char *str;
    int i;
};

extern struct Tokens *new_string(char s[]);
extern void next_char(struct Tokens *t);
extern void rewind_string(struct Tokens *t);
extern void delete_string(struct Tokens *s);

#endif