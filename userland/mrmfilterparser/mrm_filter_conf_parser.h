/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#ifndef FILTER_CONF_PARSER_H_INCLUDED
#define FILTER_CONF_PARSER_H_INCLUDED

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


struct mrm_filter_config;
int filter_file_load(struct mrm_filter_config * const /* output */, const char * /* fname */);
int filter_file_loadf(struct mrm_filter_config * const /* output */, FILE * const /* f */);

#ifdef __cplusplus
}; //extern "C" {
#endif

#endif /* #ifndef FILTER_CONF_PARSER_H_INCLUDED */
