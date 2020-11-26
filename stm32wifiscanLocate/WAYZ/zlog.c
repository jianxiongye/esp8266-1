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

#include "zlog.h"

void loge(const char *fmt, ...) {
	if(LOG_LEVEL >= LOG_ERROR){
		printf("[E]");
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\r\n");
	}
}

void logi(const char *fmt, ...) {
    if(LOG_LEVEL >= LOG_INFO){
		printf("[I]");
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\r\n");
	}
}

void logd(const char *fmt, ...) {
	if(LOG_LEVEL >= LOG_DEBUG){
		printf("[D]");
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\r\n");
	}
}

