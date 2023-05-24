//
// Created by v2ray on 2023/3/2.
//

#ifndef GPT3BOT_PROMPTUTILS_H
#define GPT3BOT_PROMPTUTILS_H

#include "../data/ExchangeHistory.h"
#include "../data/Document.h"
#include "boost/format.hpp"
#include "string"
#include "vector"

namespace prompt {

    void print_prompt(const std::string& initial_prompt, const std::shared_ptr<chat::ExchangeHistory>& chat_history,
                      const std::string& me_id, const std::string& bot_id, const unsigned int& max_length,
                      const bool& is_new_api, const bool& space_between_exchanges);
    std::string to_string(std::string initial_prompt, const chat::Messages& messages, const std::string& me_id,
                          const std::string& bot_id, const bool& add_color = false, const bool& space_between_exchanges = false);
    std::string to_string(const std::string& initial_prompt, const std::shared_ptr<chat::ExchangeHistory>& chat_history,
                          const std::string& me_id, const std::string& bot_id, const unsigned int& max_length,
                          const bool& add_color = false, const bool& space_between_exchanges = false);
    std::string construct_reference(std::string initial_prompt, const std::vector<float>& input_embeddings,
                                    const std::shared_ptr<chat::ExchangeHistory>& chat_history, const bool& search_response,
                                    const unsigned int& max_reference_length, const unsigned int& max_short_memory_length,
                                    const std::string& me_id, const std::string& bot_id);
    std::string construct_reference(std::string initial_prompt, const std::vector<float>& input_embeddings,
                                    const std::shared_ptr<std::vector<std::shared_ptr<doc::Document>>>& documents,
                                    const unsigned int& max_reference_length);
} // prompt

namespace GPT {

    std::string to_payload(std::string initial_prompt, const chat::Messages& messages, const std::string& me_id, const std::string& bot_id);
} // GPT

namespace ChatGPT {

    nlohmann::json to_payload(std::string initial_prompt, const chat::Messages& messages, const std::string& model, const std::string& me_id,
                              const std::string& bot_id);
} // ChatGPT

namespace prompt {
    template<typename ListType, typename ReturnType = ListType>
    using check_bidir_it = std::enable_if<std::is_same<typename std::iterator_traits<typename ListType::iterator>::iterator_category,
    std::bidirectional_iterator_tag>::value, ReturnType>::type;
    template<typename ListType, typename ReturnType = ListType>
    using check_ra_it = typename std::enable_if<std::is_same<typename std::iterator_traits<typename ListType::iterator>::iterator_category,
    std::random_access_iterator_tag>::value, ReturnType>::type;

    /* Return void. */
    template<typename ListType>
    inline check_bidir_it<ListType, void> erase_except_back(ListType& s, const unsigned int& keep_back_count) {
        if (s.size() > keep_back_count) {
            s.erase(s.begin(), std::prev(s.end(), keep_back_count));
        }
    }

    /* Return void. */
    template<typename ListType>
    inline check_ra_it<ListType, void> erase_except_back(ListType& s, const unsigned int& keep_back_count) {
        if (s.size() > keep_back_count) {
            s.erase(s.begin(), s.end() - keep_back_count);
        }
    }

    /* Return ListType. */
    template<typename ListType>
    inline check_bidir_it<ListType> construct_from_back(const ListType& list, const unsigned int& count) {
        if (list.size() <= count) {
            return list;
        } else {
            return ListType(std::prev(list.end(), count), list.end());
        }
    }

    /* Return ListType. */
    template<typename ListType>
    inline check_ra_it<ListType> construct_from_back(const ListType& list, const unsigned int& count) {
        if (list.size() <= count) {
            return list;
        } else {
            return ListType(list.end() - count, list.end());
        }
    }
} // prompt

#endif //GPT3BOT_PROMPTUTILS_H
