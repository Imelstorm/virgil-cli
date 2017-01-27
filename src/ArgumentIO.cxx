/**
 * Copyright (C) 2015-2016 Virgil Security Inc.
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

#include <cli/argument/ArgumentIO.h>

#include <cli/api/api.h>
#include <cli/api/Configurations.h>
#include <cli/command/Command.h>
#include <cli/error/ArgumentError.h>
#include <cli/io/Logger.h>

#include <cli/model/EncryptCredentials.h>
#include <cli/model/DecryptCredentials.h>
#include <cli/model/PasswordEncryptCredentials.h>
#include <cli/model/PasswordDecryptCredentials.h>
#include <cli/model/KeyEncryptCredentials.h>
#include <cli/model/KeyDecryptCredentials.h>

#include <cli/command/KeygenCommand.h>
#include <cli/command/KeyToPubCommand.h>
#include <cli/command/EncryptCommand.h>
#include <cli/command/DecryptCommand.h>

#include <cli/memory.h>

#include <istream>
#include <ostream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>

using namespace cli;
using namespace cli::argument;
using namespace cli::command;
using namespace cli::model;

#undef IN
#undef OUT

ArgumentIO::ArgumentIO(
        std::unique_ptr<ArgumentSource> argumentSource, std::unique_ptr<ArgumentValueSource> argumentValueSource)
        : argumentSource_(std::move(argumentSource)), argumentValueSource_(std::move(argumentValueSource)) {
    DCHECK(argumentSource_);
    DCHECK(argumentValueSource_);
}

void ArgumentIO::configureUsage(const char* usage, const ArgumentParseOptions& parseOptions) {
    argumentSource_->init(usage, parseOptions);
    argumentValueSource_->init(*argumentSource_);
}

bool ArgumentIO::hasContentInfo() const {
    auto argument = argumentSource_->read(opt::CONTENT_INFO, ArgumentImportance::Optional);
    return !argument.isEmpty();
}

bool ArgumentIO::hasNoPassword() const {
    ULOG1(INFO) << tfm::format("Check if password should be omitted.");
    auto argument = argumentSource_->read(opt::NO_PASSWORD, ArgumentImportance::Optional);
    //TODO: Add validation
    return argument.asValue().asOptionalBool();
}

bool ArgumentIO::isInteractive() const {
    auto argument = argumentSource_->read(opt::INTERACTIVE, ArgumentImportance::Optional);
    return argument.asValue().asOptionalBool();
}

FileDataSource ArgumentIO::getInputSource(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read input.");
    auto argument = argumentSource_->read(opt::IN, argumentImportance);
    //TODO: Add validation
    return getSource(argument.asValue());
}

FileDataSink ArgumentIO::getOutputSink(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Write output.");
    auto argument = argumentSource_->read(opt::OUT, argumentImportance);
    //TODO: Add validation
    return getSink(argument.asValue());
}


FileDataSource ArgumentIO::getContentInfoSource(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read content info.");
    auto argument = argumentSource_->read(opt::CONTENT_INFO, argumentImportance);
    //TODO: Add validation
    return getSource(argument.asValue());
}

FileDataSink ArgumentIO::getContentInfoSink(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Write content info.");
    auto argument = argumentSource_->read(opt::CONTENT_INFO, argumentImportance);
    //TODO: Add validation
    return getSink(argument.asValue());
}

std::vector<std::unique_ptr<EncryptCredentials>>
ArgumentIO::getEncryptCredentials(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read recipients for encryption.");
    auto argument = argumentSource_->read(arg::RECIPIENT_ID, argumentImportance);
    //TODO: Add validation
    std::vector<std::unique_ptr<EncryptCredentials>> result;
    for (const auto& argumentValue : argument.asList()) {
        auto credentials = readEncryptCredentials(argumentValue);
        result.insert(result.end(),
                std::make_move_iterator(credentials.begin()), std::make_move_iterator(credentials.end()));
    }
    return result;
}

std::vector<std::unique_ptr<DecryptCredentials>>
ArgumentIO::getDecryptCredentials(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read recipients for decryption.");
    auto argument = argumentSource_->read(arg::KEYPASS, argumentImportance);
    //TODO: Add validation
    std::vector<std::unique_ptr<DecryptCredentials>> result;
    for (const auto& argumentValue : argument.asList()) {
        auto decryptCredentials = readDecryptCredentials(argumentValue);
        result.insert(result.end(),
                std::make_move_iterator(decryptCredentials.begin()), std::make_move_iterator(decryptCredentials.end()));
    }
    return result;
}

KeyAlgorithm ArgumentIO::getKeyAlgorithm(ArgumentImportance argumentImportance) const {
    auto argument = argumentSource_->read(opt::ALGORITHM, argumentImportance);
    //TODO: Add validation
    return argumentValueSource_->readKeyAlgorithm(argument.asValue());
}

PrivateKey ArgumentIO::getPrivateKey(ArgumentImportance argumentImportance) const {
    auto argument = argumentSource_->read(opt::PRIVATE_KEY, argumentImportance);
    //TODO: Add validation
    auto privateKey = argumentValueSource_->readPrivateKey(argument.asValue());
    readPrivateKeyPassword(privateKey, argument.asValue());
    return std::move(privateKey);
}

Password ArgumentIO::getKeyPassword(ArgumentImportance argumentImportance) const {
    auto argument = argumentSource_->read(opt::PRIVATE_KEY_PASSWORD, argumentImportance);
    //TODO: Add validation
    return argumentValueSource_->readPassword(argument.asValue());
}

PublicKey ArgumentIO::getSenderKey(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read Sender's public key.");
    auto argument = argumentSource_->read(arg::RECIPIENT_ID, argumentImportance);
    //TODO: Add validation
    return readSenderKey(argument.asValue());
}

FileDataSource ArgumentIO::getSignatureSource(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read signature.");
    auto argument = argumentSource_->read(opt::SIGN, argumentImportance);
    //TODO: Add validation
    return getSource(argument.asValue());
}

Crypto::Text ArgumentIO::getCommand(ArgumentImportance argumentImportance) const {
    ULOG1(INFO) << tfm::format("Read command.");
    auto argument = argumentSource_->read(arg::COMMAND, argumentImportance);
    //TODO: Add validation
    return Crypto::Text(argument.asValue().value());
}

CardIdentity ArgumentIO::getCardIdentity(ArgumentImportance argumentImportance) const {
    ULOG2(INFO) << "Read Card's Identity.";
    auto argument = argumentSource_->read(arg::IDENTITY, argumentImportance);
    //TODO: Add validation
    return CardIdentity(argument.asValue().key(), argument.asValue().value());
}

Crypto::Text ArgumentIO::getCardScope(ArgumentImportance argumentImportance) const {
    ULOG2(INFO) << "Read Card's Scope.";
    auto argument = argumentSource_->read(opt::SCOPE, argumentImportance);
    //TODO: Add validation
    return argument.asValue().asString();
}

CardData ArgumentIO::getCardData(ArgumentImportance argumentImportance) const {
    ULOG2(INFO) << "Read Card's Data.";
    auto argument = argumentSource_->read(opt::DATA, argumentImportance);
    //TODO: Add validation
    CardData cardData;
    for (const auto& argumentValue : argument.asList()) {
        cardData[argumentValue.key()] = argumentValue.value();
    }
    return cardData;
}

CardInfo ArgumentIO::getCardInfo(ArgumentImportance argumentImportance) const {
    ULOG2(INFO) << "Read Card's Info.";
    auto argument = argumentSource_->read(opt::INFO, argumentImportance);
    //TODO: Add validation
    std::string device;
    std::string deviceName;
    for (const auto& argumentValue : argument.asList()) {
        auto infoKey = argumentValue.key();
        if (infoKey == arg::value::VIRGIL_CARD_CREATE_INFO_KEY_DEVICE) {
            device = argumentValue.value();
        } else if (infoKey == arg::value::VIRGIL_CARD_CREATE_INFO_KEY_DEVICE_NAME) {
            deviceName = argumentValue.value();
        } else {
            throw error::ArgumentInvalidKey(infoKey, arg::value::VIRGIL_CARD_CREATE_INFO_KEY_VALUES);
        }
    }
    return CardInfo(device, deviceName);
}

SecureValue ArgumentIO::getAppAccessToken(ArgumentImportance argumentImportance) const {
    ULOG2(INFO) << "Read Application Access Token.";
    auto argument = argumentSource_->read(arg::value::VIRGIL_CONFIG_APP_ACCESS_TOKEN, argumentImportance);
    //TODO: Add validation
    return SecureValue(argument.asValue().asString());
}

ApplicationCredentials ArgumentIO::getAppCredentials(ArgumentImportance argumentImportance) const {
    ULOG2(INFO) << "Read Application Credentials (identifier, private key, private key password).";
    auto argumentAppKeyId = argumentSource_->read(arg::value::VIRGIL_CONFIG_APP_KEY_ID, argumentImportance);
    auto argumentAppKeyData = argumentSource_->read(arg::value::VIRGIL_CONFIG_APP_KEY_DATA, argumentImportance);
    auto argumentAppKeyPassword = argumentSource_->read(arg::value::VIRGIL_CONFIG_APP_KEY_PASSWORD, argumentImportance);
    //TODO: Add validation
    return ApplicationCredentials(
            SecureValue(argumentAppKeyId.asValue().origin()),
            SecureValue(argumentAppKeyData.asValue().origin()),
            SecureValue(argumentAppKeyPassword.asValue().origin())
    );
}


FileDataSource ArgumentIO::getSource(const ArgumentValue& argumentValue) const {
    if (argumentValue.isEmpty()) {
        ULOG1(INFO) << tfm::format("Read input from: standard input.");
        return FileDataSource();
    } else {
        ULOG1(INFO) << tfm::format("Read input from file: '%s'.", argumentValue.value());
        return FileDataSource(argumentValue.value());
    }
}

FileDataSink ArgumentIO::getSink(const ArgumentValue& argumentValue) const {
    if (argumentValue.isEmpty()) {
        ULOG1(INFO) << tfm::format("Write to the standard output.");
        return FileDataSink();
    } else {
        ULOG1(INFO) << tfm::format("Write to the file: '%s'.", argumentValue.value());
        return FileDataSink(argumentValue.value());
    }
}

void ArgumentIO::readPrivateKeyPassword(PrivateKey& privateKey, const ArgumentValue& argumentValue) const {
    if (!privateKey.isEncrypted()) {
        return;
    }
    std::string passwordOption = opt::PRIVATE_KEY_PASSWORD;
    do {
        ULOG1(INFO) << tfm::format("Read password for the private key: '%s'.", std::to_string(argumentValue));
        auto argument = argumentSource_->read(passwordOption.c_str(), ArgumentImportance::Required);
        auto password = argumentValueSource_->readPassword(argument.asValue());
        if (privateKey.checkPassword(password)) {
            privateKey.setPassword(std::move(password));
            return;
        }
        passwordOption = argumentValue.value();
    } while (isInteractive());
    throw error::ArgumentRuntimeError(
            tfm::format("Wrong password for the private key '%s'.", std::to_string(argumentValue)));
}

std::vector<std::unique_ptr<EncryptCredentials>>
ArgumentIO::readEncryptCredentials(const ArgumentValue& argumentValue) const {
    ULOG1(INFO) << tfm::format("Read recipient(s) from the token: '%s'.", std::to_string(argumentValue));
    auto recipientType = argumentValue.key();
    std::vector<std::unique_ptr<EncryptCredentials>> result;
    if (recipientType == arg::value::VIRGIL_ENCRYPT_RECIPIENT_ID_PASSWORD) {
        //TODO: Add validation
        result.push_back(std::make_unique<PasswordEncryptCredentials>(
                argumentValueSource_->readPassword(argumentValue)
        ));
    } else if (recipientType == arg::value::VIRGIL_ENCRYPT_RECIPIENT_ID_PUBKEY) {
        //TODO: Add validation
        result.push_back(std::make_unique<KeyEncryptCredentials>(
                argumentValueSource_->readPublicKey(argumentValue)
        ));
    } else if (recipientType == arg::value::VIRGIL_ENCRYPT_RECIPIENT_ID_VCARD ||
            recipientType == arg::value::VIRGIL_ENCRYPT_RECIPIENT_ID_EMAIL) {
        //TODO: Add validation
        auto cards = argumentValueSource_->readCards(argumentValue);
        for (auto&& card : cards) {
            result.push_back(std::make_unique<KeyEncryptCredentials>(std::move(card)));
        }
    } else {
        throw error::ArgumentInvalidKey(recipientType, arg::value::VIRGIL_ENCRYPT_RECIPIENT_ID_VALUES);
    }
    return result;
}

std::vector<std::unique_ptr<DecryptCredentials>>
ArgumentIO::readDecryptCredentials(const ArgumentValue& argumentValue) const {
    ULOG1(INFO) << tfm::format("Read recipient(s) from the token: '%s'.", std::to_string(argumentValue));
    auto recipientType = argumentValue.key();
    std::vector<std::unique_ptr<DecryptCredentials>> result;
    if (recipientType == arg::value::VIRGIL_DECRYPT_KEYPASS_PASSWORD) {
        result.push_back(std::make_unique<PasswordDecryptCredentials>(
                argumentValueSource_->readPassword(argumentValue)
        ));
    } else if (recipientType == arg::value::VIRGIL_DECRYPT_KEYPASS_PRIVKEY) {
        auto privateKey = argumentValueSource_->readPrivateKey(argumentValue);
        readPrivateKeyPassword(privateKey, argumentValue);
        result.push_back(std::make_unique<KeyDecryptCredentials>(std::move(privateKey)));
    } else {
        throw error::ArgumentInvalidKey(recipientType, arg::value::VIRGIL_DECRYPT_KEYPASS_VALUES);
    }
    return result;
}

PublicKey ArgumentIO::readSenderKey(const ArgumentValue& argumentValue) const {
    if (argumentValue.key() == arg::value::VIRGIL_VERIFY_RECIPIENT_ID_PUBKEY) {
        return argumentValueSource_->readPublicKey(argumentValue);
    } else if (argumentValue.key() == arg::value::VIRGIL_VERIFY_RECIPIENT_ID_VCARD) {
        auto cards = argumentValueSource_->readCards(argumentValue);
        CHECK(cards.size() == 1);
        auto card = cards.front();
        return PublicKey(card.publicKeyData(), card.identifier());
    }
    throw error::ArgumentInvalidKey(argumentValue.key(), arg::value::VIRGIL_VERIFY_RECIPIENT_ID_VALUES);
}
