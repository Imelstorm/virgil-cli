/**
 * Copyright (C) 2015 Virgil Security Inc.
 *
 * Lead Maintainer: Virgil Security Inc. <support@virgilsecurity.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *     (3) Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdexcept>
#include <string>
#include <vector>

#include <tclap/CmdLine.h>

#include <virgil/crypto/VirgilByteArray.h>

#include <virgil/sdk/ServicesHub.h>
#include <virgil/sdk/io/Marshaller.h>

#include <cli/version.h>
#include <cli/config.h>
#include <cli/pair.h>
#include <cli/util.h>

namespace vsdk = virgil::sdk;
namespace vcrypto = virgil::crypto;
namespace vcli = virgil::cli;

#ifdef SPLIT_CLI
#define MAIN main
#else
#define MAIN card_search_main
#endif

int MAIN(int argc, char** argv) {
    try {
        std::string description = "Search a Virgil Card from the Virgil Keys service \n";

        std::vector<std::string> examples;
        examples.push_back("Search Virgil Cards with confirm identity:\n"
                           "virgil card-search -d email:user_email");

        examples.push_back("Search Virgil Cards include unconfirm identity:\n"
                           "virgil card-search -d email:user_email -u");

        examples.push_back(
            "Search Virgil Cards with confirm identity signed other Cards <user1_card_id> <user2_card_id>:\n"
            "virgil card-search -d email:user_email "
            "<user1_card_id> <user1_card_id>");

        examples.push_back(
            "Search Virgil Cards with unconfirm identity signed other Cards <user1_card_id> <user2_card_id>:\n"
            "virgil card-search -d email:user_email -u "
            "<user1_card_id> <user1_card_id>");

        std::string descriptionMessage = virgil::cli::getDescriptionMessage(description, examples);

        // Parse arguments.
        TCLAP::CmdLine cmd(descriptionMessage, ' ', virgil::cli_version());

        TCLAP::ValueArg<std::string> outArg("o", "out", "Virgil Cards. If omitted stdout is used.", false, "", "file");

        TCLAP::ValueArg<std::string> identityArg("d", "identity", "Identity: email", true, "", "arg");

        TCLAP::SwitchArg unconfirmedArg("u", "unconfirmed", "Search Cards include unconfirm identity", false);

        TCLAP::UnlabeledMultiArg<std::string> signedCardsIdArg("signed-card-id", "Signed card id", false, "card-id",
                                                               false);

        cmd.add(signedCardsIdArg);
        cmd.add(unconfirmedArg);
        cmd.add(identityArg);
        cmd.add(outArg);
        cmd.parse(argc, argv);

        auto identityPair = vcli::parsePair(identityArg.getValue());
        std::string arg = "-d, --identity";
        vcli::checkFormatIdentity(arg, identityPair.first);
        std::string userEmail = identityPair.second;
        vsdk::dto::Identity identity(userEmail, vsdk::models::IdentityModel::Type::Email);

        bool includeUnconfirmed = unconfirmedArg.getValue();

        vsdk::ServicesHub servicesHub(VIRGIL_ACCESS_TOKEN);
        std::vector<vsdk::models::CardModel> foundCards;
        if (signedCardsIdArg.isSet()) {
            std::vector<std::string> signedCardsId = signedCardsIdArg.getValue();
            foundCards = servicesHub.card().search(identity, includeUnconfirmed, signedCardsId);
        } else {
            foundCards = servicesHub.card().search(identity, includeUnconfirmed);
        }

        std::string foundCardsStr = vsdk::io::cardsToJson(foundCards, 4);
        vcli::writeBytes(outArg.getValue(), foundCardsStr);

        if (foundCards.empty()) {
            std::cout << "Cards by email: " << userEmail << " не найдены" << std::endl;
        } else {
            std::cout << "Cards by email: " << userEmail << " получены" << std::endl;
        }

    } catch (TCLAP::ArgException& exception) {
        std::cerr << "card-search. Error: " << exception.error() << " for arg " << exception.argId() << std::endl;
        return EXIT_FAILURE;
    } catch (std::exception& exception) {
        std::cerr << "card-search. Error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
