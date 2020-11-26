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

#ifndef __TRACK_DEBUG_H__
#define __TRACK_DEBUG_H__

//#define FLAG_PRETTY_JSON
#include <stdint.h>





#ifndef int64
#define int64 int64_t
#endif

#ifndef int32
#define int32 int32_t
#endif

typedef enum{false,true} bool;
/*******
********
********
*******/
typedef struct {
    double longitude;
	double latitude;
	float  altitude;
} Point;

// os信息
typedef struct iot_os_st {
    // 机型
    const char *type;
    // 版本
    const char *version;
} iot_os_t;
/*******
********
*******/
 

typedef struct iot_cellulars_st{
    int64 timestamp;
	int32 cellId;
	bool connected;
	int32 signalStrength;
	char *radioType;
	int32 mobileCountryCode;
	int32 mobileNetworkCode;
	int32 locationAreaCode;
} iot_cellulars_t;

typedef struct iot_wifis_st{
	//int64 timestamp;
    char macAddress[18];
	char ssid[50];
	int32 signalStrength;
	int32 frequency;
	int32 channel;
	int32 connected;
	int32 wificount;
	
} iot_wifis_t;

typedef struct {
    char *Connection;
} iot_connection_t;

typedef struct iot_bluetooth_st{
    char *Bluetooth;
	char *name;
	int32 signalStrength;
} iot_bluetooth_t;

//位置数据
typedef struct {
    int64 timestamp;
	const char *source;
	Point point;
	const char  *spatialReference;
	float accuracy;
	float verticalAccuracy;
	float velocity;
	float velocityAccuracy;
	int32 heading;
	int32 headingAccuracy;
} iot_position_t;

// gps 信息
typedef struct iot_gnss_st {
	Point point;
    // 时间戳
    int64 timestamp;
    // 是否合法 0合法 1不合法
    int valid;
    // 精准度
    double accuracy;
	// 方向
    int32 heading;
	//可以不用下面三个参数
    // 速度
    double velocity;
	//垂直精度
	float verticalAccuracy;
	//速度精度
	float velocityAccuracy;

} iot_gnss_t;




/*************
*************/
// 设备信息
typedef struct iot_asset_st{
    // 设备id
    const char *id;
    // 设备品牌
    const char *manufacturer;
    // 设备
    const char *model;
    // 设备
    const char *imei;
    // mac
    const char *macAddress;
	//SN号
	const char *serialNumber;
	//系统提供的唯一码，Android系统的android_id，iOS 系统的 IDFA
	const char *uniqueId;

    iot_os_t *os;
} iot_asset_t;

// location 位置信息
typedef struct iot_location_st {
    iot_gnss_t *gnss;
	iot_cellulars_t *cellulars;//n

	iot_connection_t connection;//1
	
	iot_position_t position;
		
		iot_bluetooth_t *bluetooths;//n
		iot_wifis_t *wifis;//n
} iot_location_t;

/********************/
// track 信息
typedef struct iot_track_point_st {
    int64 timestamp;
    iot_asset_t *asset;
	char *id;
    iot_location_t *location;
} iot_track_point_t;

/// 构建iot_gnss_t
/// @return iot_gnss_t
iot_gnss_t *create_gnss(double lng, double lat, double alt);

/// 构建iot_location_t
/// @return iot_location_t
iot_location_t *create_location(iot_gnss_t *gnss);

/// 构建track
/// @return iot_track_point_t
iot_track_point_t *create_track(iot_asset_t *asset, iot_location_t *location);

/// 构建设备信息 必须传asset id
/// @return iot_asset_t
iot_asset_t *create_asset(const char *asset_id);

/// 构建系统信息
/// @return iot_os_t
iot_os_t *create_os(const char *type, const char *version);

/// 回收track
void destroy_track(iot_track_point_t *track);

/// 回收gnss
void destroy_gnss(iot_gnss_t *gnss);

/// 回收asset
void destroy_asset(iot_asset_t *asset);

/// 回收os
void destroy_os(iot_os_t *os);

/// 回收系统信息
void destroy_location(iot_location_t *location);

#endif
