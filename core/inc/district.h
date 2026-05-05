#ifndef DISTRICT_H

#define DISTRICT_H

#include "commons.h"
#include "utils.h"

int create_district(char* districtID);
int update_threshold(char* districtID, char* value);
int remove_district(char* districtID);

#endif // DISTRICT_H