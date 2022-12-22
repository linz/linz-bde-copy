//
// Created by AEmad on 19/12/2022.
//
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <iostream>
#include <memory>
#include "bde_copy.h"
#include <aws/s3/model/PutObjectRequest.h>
#include <dirent.h> // for opendir, readdir
#include <fstream>
#include <string>


using namespace aws::lambda_runtime;

bool PutObjectBuffer(Aws::S3::S3Client const& client,
                     const Aws::String &bucket,
                     const Aws::String &objectName,
                     const std::string &objectContent);

std::string download_file(
        Aws::S3::S3Client const& client,
        Aws::String const& bucket,
        Aws::String const& key);


std::string load_updated_file();

char const TAG[] = "LAMBDA_ALLOC";


static invocation_response my_handler(invocation_request const& req, Aws::S3::S3Client const& client)
{
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful()) {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    if (!v.ValueExists("s3bucket") || !v.ValueExists("s3key") || !v.ValueExists("s3newKey") || !v.ValueExists("s3newBucket")
    || !v.GetObject("s3bucket").IsString() || !v.GetObject("s3key").IsString()  || !v.GetObject("s3newKey").IsString() ||
            !v.GetObject("s3newBucket").IsString()) {
        return invocation_response::failure("Missing input value s3bucket or s3key", "InvalidJSON");
    }

    auto bucket = v.GetString("s3bucket");
    auto key = v.GetString("s3key");
    auto new_key = v.GetString("s3newKey");
    auto new_bucket = v.GetString("s3newBucket");


    // download file from s3
     auto err = download_file(client, bucket, key);


    size_t last_slash_pos = key.find_last_of('/');
    if (last_slash_pos != std::string::npos) {
        std::string file_name = key.substr(last_slash_pos + 1);
        str:std::string full_file_name = "/tmp/" + file_name;
        // call bde processor
        char *argv[] = { "program", const_cast<char*>(full_file_name.c_str()), "/tmp/bde_generated_file.csv", NULL };
        main_bde_copy(3, argv);


        // retrieve the generated bde file

        Aws::String file_content =  load_updated_file();
        // put it to s3 bucket
        PutObjectBuffer(client, new_bucket, new_key, file_content);
    }


    if (!err.empty()) {
        return invocation_response::failure(err, "DownloadFailure");
    }

    return invocation_response::success("success", "application/json");
}



int main()
{

    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
//    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {
        Client::ClientConfiguration config;
        config.region = "ap-southeast-2";
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

        auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
        S3::S3Client client(config);
        auto handler_fn = [&client](aws::lambda_runtime::invocation_request const& req) {
            return my_handler(req, client);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;


}

std::string download_file(
        Aws::S3::S3Client const& client,
        Aws::String const& bucket,
        Aws::String const& key)
{
    using namespace Aws;

    S3::Model::GetObjectRequest request;
    request.WithBucket(bucket).WithKey(key);

    auto outcome = client.GetObject(request);
    if (outcome.IsSuccess()) {
        auto& s = outcome.GetResult().GetBody();
        size_t last_slash_pos = key.find_last_of('/');
        if (last_slash_pos != std::string::npos) {
            std::string file_name = key.substr(last_slash_pos + 1);
            Aws::OFStream local_file("/tmp/"+file_name, std::ios::out | std::ios::binary);
            local_file << outcome.GetResult().GetBody().rdbuf();
            return "success";
        }  else {
            return "failure";
        }

    }
    else {
        AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome.GetError());
        return outcome.GetError().GetMessage();
    }
}

std::string load_updated_file() {
    std::ifstream file("/tmp/bde_generated_file.csv", std::ios::binary);
    std::string objectContent((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

    return objectContent;

}
bool PutObjectBuffer(Aws::S3::S3Client const& client,
                     const Aws::String &bucket,
                     const Aws::String &objectName,
                     const std::string &objectContent){


    using namespace Aws;

    S3::Model::PutObjectRequest request;
    request.WithBucket(bucket).WithKey(objectName);

    const std::shared_ptr<Aws::IOStream> inputData =
            Aws::MakeShared<Aws::StringStream>("");
    *inputData << objectContent.c_str();

    request.SetBody(inputData);

    auto outcome = client.PutObject(request);

    if (!outcome.IsSuccess()) {
        std::cerr << "Error: PutObjectBuffer: " <<
                  outcome.GetError().GetMessage() << std::endl;
    }
    else {
        std::cout << "Success: Object '" << objectName << "'uploaded to bucket '" << bucket << "'.";
    }

    return outcome.IsSuccess();
}