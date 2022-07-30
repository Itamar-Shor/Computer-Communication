#pragma once

//split str according to delim as the delimeter.
//return the number of splits
int split(char* str, const char delim, int max_splits, char** dst);

//packet is the string representing the packet.
//field is the field to be extracted from packet (e.g. time, weight).
char* extract_field(char* packet, int field);