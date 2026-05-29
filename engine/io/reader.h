#ifndef PARGUS_READER_H
#define PARGUS_READER_H

#include "common/types.h"
#include "config/config.h"

int read_documents_from_dir(const char *input_dir, const EngineConfig *config, DocumentList *docs);

#endif
