#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "ydb/public/api/grpc/ydb_operation_v1.grpc.pb.h"
#include "ydb/public/api/grpc/ydb_discovery_v1.grpc.pb.h"
#include "ydb/public/api/grpc/ydb_table_v1.grpc.pb.h"
#include "ydb/public/api/protos/ydb_operation.pb.h"
#include "ydb/public/api/protos/ydb_discovery.pb.h"
#include "ydb/public/api/protos/ydb_table.pb.h"
#include "ydb/public/api/protos/ydb_value.pb.h"
#include "ydb/public/api/protos/ydb_status_codes.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
// using grpc::ServerWriter;

class MockOperationService final : public Ydb::Operation::V1::OperationService::Service {
public:
    Status GetOperation(
        ServerContext* /*context*/,
        const Ydb::Operations::GetOperationRequest* /*request*/,
        Ydb::Operations::GetOperationResponse* response
    ) override {
        auto* operation = new Ydb::Operations::Operation();
        operation->set_ready(true);
        operation->set_status(Ydb::StatusIds::SUCCESS);

        response->set_allocated_operation(operation);
        return Status::OK;
    }
};

class MockDiscoveryService final : public Ydb::Discovery::V1::DiscoveryService::Service {
public:
    MockDiscoveryService(const std::string& address, int port)
        : address_(address), port_(port) {}

    Status ListEndpoints(ServerContext* /*context*/, const Ydb::Discovery::ListEndpointsRequest* /*request*/,
                        Ydb::Discovery::ListEndpointsResponse* response) override {
        // Use stack allocation for better memory safety
        Ydb::Discovery::EndpointInfo endpoint;
        endpoint.set_address(address_);
        endpoint.set_port(port_);

        Ydb::Discovery::ListEndpointsResult result;
        result.mutable_endpoints()->AddAllocated(new Ydb::Discovery::EndpointInfo(endpoint));

        auto* operation = new Ydb::Operations::Operation();
        operation->set_ready(true);
        operation->set_status(Ydb::StatusIds::SUCCESS);
        operation->mutable_result()->PackFrom(result);  // Makes a copy of result

        response->set_allocated_operation(operation);  // Transfers ownership to response
        return Status::OK;
    }

private:
    std::string address_;
    int port_;
};

class MockTableService final : public Ydb::Table::V1::TableService::Service {
public:
    Status DescribeTable(ServerContext* /*context*/, const Ydb::Table::DescribeTableRequest* /*request*/,
                        Ydb::Table::DescribeTableResponse* response) override {
        auto* result = new Ydb::Table::DescribeTableResult();

        // Add columns as per Java implementation
        AddColumn(result, "event_time", Ydb::Type::TZ_DATETIME, true);
        AddColumn(result, "event_type", Ydb::Type::STRING, true);
        AddColumn(result, "product_id", Ydb::Type::STRING, true);
        AddColumn(result, "category_id", Ydb::Type::STRING, true);
        AddColumn(result, "category_code", Ydb::Type::STRING, false);
        AddColumn(result, "brand", Ydb::Type::STRING, false);
        AddColumn(result, "price", Ydb::Type::DOUBLE, true);
        AddColumn(result, "user_id", Ydb::Type::STRING, true);
        AddColumn(result, "user_session", Ydb::Type::STRING, true);

        auto* operation = new Ydb::Operations::Operation();
        operation->set_ready(true);
        operation->set_status(Ydb::StatusIds::SUCCESS);
        operation->mutable_result()->PackFrom(*result);

        response->set_allocated_operation(operation);
        delete result;
        return Status::OK;
    }

    Status CreateSession(ServerContext* /*context*/, const Ydb::Table::CreateSessionRequest* /*request*/,
                        Ydb::Table::CreateSessionResponse* response) override {
        auto* result = new Ydb::Table::CreateSessionResult();
        result->set_session_id("test_session_id");

        auto* operation = new Ydb::Operations::Operation();
        operation->set_ready(true);
        operation->set_status(Ydb::StatusIds::SUCCESS);
        operation->mutable_result()->PackFrom(*result);

        response->set_allocated_operation(operation);
        delete result;
        return Status::OK;
    }

    Status BulkUpsert(ServerContext* /*context*/, const Ydb::Table::BulkUpsertRequest* /*request*/,
                      Ydb::Table::BulkUpsertResponse* /*response*/) override {
        // Empty response as per Java implementation
        return Status::OK;
    }

private:
    void AddColumn(Ydb::Table::DescribeTableResult* result, const std::string& name,
                  Ydb::Type::PrimitiveTypeId type_id, bool not_null) {
        auto* column = result->add_columns();
        column->set_name(name);
        column->mutable_type()->set_type_id(type_id);
        column->set_not_null(not_null);
    }
};

int main(int /*argc*/, char** /*argv*/) {
    std::string address = "localhost";
    int port = 50051;
    std::string server_address = address + ":" + std::to_string(port);

    grpc::EnableDefaultHealthCheckService(true);

    ServerBuilder builder;
    builder.AddListeningPort(server_address.c_str(), grpc::InsecureServerCredentials());

    // Add services
    MockTableService table_service;
    MockDiscoveryService discovery_service(address, port);
    MockOperationService operation_service;

    builder.RegisterService(&table_service);
    builder.RegisterService(&discovery_service);
    builder.RegisterService(&operation_service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server started, listening on " << server_address << std::endl;

    server->Wait();
    return 0;
}
