//
// Created by v2ray on 2023/4/26.
//

#ifndef GPT3BOT_FINETUNEHELPER_H
#define GPT3BOT_FINETUNEHELPER_H

#include "GPTMain.h"
#include "interface/model/FineTune.h"
#include "thread"

namespace fth {

    void fine_tune_helper_main(const config::Config& config);
} // fth

#endif //GPT3BOT_FINETUNEHELPER_H
