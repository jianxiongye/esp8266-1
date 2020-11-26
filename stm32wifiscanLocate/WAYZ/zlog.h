/*
 * Copyright (c) 2018-2019 Wayz. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <stdarg.h>

//0:off 1:error 2:error&info 3.error&info&debug
#define LOG_OFF   0
#define LOG_ERROR 1
#define LOG_INFO  2
#define LOG_DEBUG 3
#define LOG_ALL   LOG_DEBUG

//we suggest develop and debug Use LOG_ALL,
//release Use LOG_OFF or LOG_ERROR
#define LOG_LEVEL LOG_ALL

void loge(const char *fmt, ...);
void logi(const char *fmt, ...);
void logd(const char *fmt, ...);


#endif
