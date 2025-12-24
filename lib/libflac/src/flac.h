/**
 * @file flac.h
 * @author Phil Schatzmann
 * @brief  Arduino include file which allows access to subfolder includes
 * @version 0.1
 * @date 2022-05-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once
// depends on https://github.com/pschatzmann/codec-ogg
#include "ogg.h"
#include "share/compat.h"
#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"
#include "FLAC/metadata.h"
