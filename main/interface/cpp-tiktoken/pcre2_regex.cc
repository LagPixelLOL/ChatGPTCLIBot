/*
 * Copyright (c) 2023 by Mark Tarrabain All rights reserved. Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the nor the names of its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pcre2_regex.h"
#include <stdexcept>

PCRERegex::PCRERegex(const std::string &pattern, int flags)
{
    int error = 0;
    PCRE2_SIZE error_offset = 0;
    flags |= PCRE2_UCP | PCRE2_UTF;
    regex_ = pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(pattern.c_str()), pattern.size(), static_cast<uint32_t>(flags), &error, &error_offset, nullptr);
    if (!regex_) {
        char buffer[512];
        pcre2_get_error_message_8(error, reinterpret_cast<PCRE2_UCHAR8 *>(buffer), sizeof(buffer));
        throw std::runtime_error(buffer);
    }
}

PCRERegex::~PCRERegex()
{
    pcre2_code_free_8(regex_);
}

std::vector<std::string> PCRERegex::all_matches(const std::string &text) const
{
    std::vector<std::string> matches;
    PCRE2_SPTR8 text_ptr = reinterpret_cast<PCRE2_SPTR8>(text.c_str());
    PCRE2_SIZE text_length = text.size();
    pcre2_match_data_8* match_data = pcre2_match_data_create_from_pattern_8(regex_, nullptr);
    int start_offset = 0;
    int match_length;
    int rc;
    do {
        rc = pcre2_match_8(regex_, text_ptr, text_length, start_offset, 0, match_data, nullptr);
        if (rc >= 0) {
            PCRE2_SIZE* ovector = pcre2_get_ovector_pointer_8(match_data);
            PCRE2_SPTR8 match_ptr = text_ptr + ovector[0];
            match_length = ovector[1] - ovector[0];
            matches.emplace_back(reinterpret_cast<const char*>(match_ptr), match_length);
            start_offset = ovector[1];
        }
    } while (rc >= 0 && start_offset < text_length && match_length > 0);
    pcre2_match_data_free_8(match_data);
    return matches;
}
