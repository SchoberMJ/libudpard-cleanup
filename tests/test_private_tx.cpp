// This software is distributed under the terms of the MIT License.
// Copyright (c) 2016-2020 OpenCyphal Development Team.
/// Copyright 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.

#include "exposed.hpp"
#include "helpers.hpp"
#include "catch/catch.hpp"

#define ETHARD_SUBJECT_ID_PORT 16383U

TEST_CASE("SessionSpecifier")
{
    // Message
    EthardSessionSpecifier specifier = {};
    REQUIRE(0 == exposed::txMakeMessageSessionSpecifier(0b0110011001100, 0b0100111, 0xc0a80000, &specifier));
    REQUIRE(ETHARD_SUBJECT_ID_PORT == specifier.data_specifier);
    REQUIRE(0b11101111'0'0101000'000'0110011001100 == specifier.destination_route_specifier);
    REQUIRE(0b11000000'10101000'00000000'00100111 == specifier.source_route_specifier);
    // Service Request
    REQUIRE(0 ==
            exposed::txMakeServiceSessionSpecifier(0b0100110011, true, 0b1010101, 0b0101010, 0xc0a80000, &specifier));
    REQUIRE(16998 == specifier.data_specifier);
    REQUIRE(0b11000000'10101000'00000000'00101010 == specifier.destination_route_specifier);
    REQUIRE(0b11000000'10101000'00000000'01010101 == specifier.source_route_specifier);
    // Service Response
    REQUIRE(0 ==
            exposed::txMakeServiceSessionSpecifier(0b0100110011, false, 0b1010101, 0b0101010, 0xc0a80000, &specifier));
    REQUIRE(16999 == specifier.data_specifier);
    REQUIRE(0b11000000'10101000'00000000'00101010 == specifier.destination_route_specifier);
    REQUIRE(0b11000000'10101000'00000000'01010101 == specifier.source_route_specifier);
}

TEST_CASE("adjustPresentationLayerMTU") {}

TEST_CASE("txMakeSessionSpecifier")
{
    using exposed::txMakeSessionSpecifier;

    EthardTransferMetadata meta{};
    EthardSessionSpecifier specifier{};

    const auto mk_meta = [&](const EthardPriority     priority,
                             const EthardTransferKind kind,
                             const std::uint16_t      port_id,
                             const std::uint8_t       remote_node_id) {
        meta.priority       = priority;
        meta.transfer_kind  = kind;
        meta.port_id        = port_id;
        meta.remote_node_id = remote_node_id;
        return &meta;
    };

    union PriorityAlias
    {
        std::uint8_t   bits;
        EthardPriority prio;
    };
    /*
    int32_t txMakeSessionSpecifier(const EthardTransferMetadata* const tr,
                                   const EthardNodeID                  local_node_id,
                                   const EthardIPv4Addr                local_node_addr,
                                   const size_t                        presentation_layer_mtu,
                                   EthardSessionSpecifier* const       spec);
    */

    // MESSAGE TRANSFERS
    REQUIRE(0 ==  // Regular message.
            txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional,
                                           EthardTransferKindMessage,
                                           0b1001100110011,
                                           ETHARD_NODE_ID_UNSET),
                                   0b1010101,
                                   0xc0a80000,
                                   &specifier));

    REQUIRE(ETHARD_SUBJECT_ID_PORT == specifier.data_specifier);
    REQUIRE(0b11101111'0'0101000'000'1001100110011 == specifier.destination_route_specifier);
    REQUIRE(0b11000000'10101000'00000000'01010101 == specifier.source_route_specifier);
    REQUIRE(-ETHARD_ERROR_INVALID_ARGUMENT ==  // Invalid node-ID
            txMakeSessionSpecifier(mk_meta(EthardPriorityImmediate,
                                           EthardTransferKindMessage,
                                           0b1001100110011,
                                           ETHARD_NODE_ID_UNSET),
                                   0xBEEF,  // node-ID too large
                                   0xc0a80000,
                                   &specifier));
    REQUIRE(-ETHARD_ERROR_INVALID_ARGUMENT ==  // Bad subject-ID.
            txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional,
                                           EthardTransferKindMessage,
                                           0xFFFFU,
                                           ETHARD_NODE_ID_UNSET),
                                   0b1010101,
                                   0xc0a80000,
                                   &specifier));

    REQUIRE(-ETHARD_ERROR_INVALID_ARGUMENT ==  // Bad priority.
            txMakeSessionSpecifier(mk_meta(PriorityAlias{123}.prio,
                                           EthardTransferKindMessage,
                                           0b1001100110011,
                                           ETHARD_NODE_ID_UNSET),
                                   0b1010101,
                                   0xc0a80000,
                                   &specifier));

    // SERVICE TRANSFERS
    REQUIRE(
        0 ==  // Request.
        txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional, EthardTransferKindRequest, 0b0100110011, 0b0101010),
                               0b1010101,
                               0xc0a80000,
                               &specifier));
    REQUIRE(
        0 ==  // Response.
        txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional, EthardTransferKindResponse, 0b0100110011, 0b0101010),
                               0b1010101,
                               0xc0a80000,
                               &specifier));
    REQUIRE(
        -ETHARD_ERROR_INVALID_ARGUMENT ==  // Anonymous source service transfers not permitted.
        txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional, EthardTransferKindRequest, 0b0100110011, 0b0101010),
                               ETHARD_NODE_ID_UNSET,
                               0xc0a80000,
                               &specifier));

    REQUIRE(-ETHARD_ERROR_INVALID_ARGUMENT ==  // Anonymous destination service transfers not permitted.
            txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional,
                                           EthardTransferKindResponse,
                                           0b0100110011,
                                           ETHARD_NODE_ID_UNSET),
                                   0b1010101,
                                   0xc0a80000,
                                   &specifier));

    REQUIRE(-ETHARD_ERROR_INVALID_ARGUMENT ==  // Bad service-ID.
            txMakeSessionSpecifier(mk_meta(EthardPriorityExceptional, EthardTransferKindResponse, 0xFFFFU, 0b0101010),
                                   0b1010101,
                                   0xc0a80000,
                                   &specifier));
    REQUIRE(
        -ETHARD_ERROR_INVALID_ARGUMENT ==  // Bad priority.
        txMakeSessionSpecifier(mk_meta(PriorityAlias{123}.prio, EthardTransferKindResponse, 0b0100110011, 0b0101010),
                               0b1010101,
                               0xc0a80000,
                               &specifier));
}

TEST_CASE("txMakeTailByte") {}

TEST_CASE("txRoundFramePayloadSizeUp") {}