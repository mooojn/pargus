#ifndef PARGUS_WRITER_H
#define PARGUS_WRITER_H

#include "common/types.h"
#include "config/config.h"
#include "nlp/similarity.h"

int write_engine_outputs(const EngineConfig *config, const DocumentList *docs, const SimilarityResults *similarity, StageTimings *timings);

#endif
