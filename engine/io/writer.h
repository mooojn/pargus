#ifndef PARGUS_WRITER_H
#define PARGUS_WRITER_H

#include "common/types.h"
#include "config/config.h"

int write_placeholder_outputs(const EngineConfig *config, const DocumentList *docs, const StageTimings *timings);

#endif

