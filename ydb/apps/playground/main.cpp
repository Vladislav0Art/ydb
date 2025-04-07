
#include <iostream>
#include <memory>
#include <iomanip>

#include <library/cpp/string_utils/csv/csv.h>
#include <util/generic/string.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/length.h>
#include <util/system/file.h>
#include <util/datetime/base.h>
#include <util/generic/size_literals.h>
#include <util/stream/format.h>



TString PrettifyBytes(double bytes) {
    return ToString(HumanReadableSize(bytes, SF_BYTES));
}


void testTLinesSplitter(const TString& filePath) {
    const TFsPath dataFile(filePath);

    if (!dataFile.Exists() || !dataFile.IsFile()) {
        std::cerr << "Not a file or file doesn't exist: " << filePath << std::endl;
        exit(EXIT_FAILURE);
    }

    TFile file(filePath, OpenExisting | RdOnly | Seq);
    const i64 fileLength = file.GetLength();

    std::cout << "File length: " << fileLength << std::endl;

    const ui64 fileBufferSize = 2_MB;
    std::unique_ptr<TFileInput> fileInput = std::make_unique<TFileInput>(file, fileBufferSize);

    IInputStream& input = *fileInput;

    TCountingInput countInput(&input);
    NCsvFormat::TLinesSplitter splitter(countInput);

    ui64 totalBytesRead = 0;
    auto start = TInstant::Now();

    while (TString line = splitter.ConsumeLine()) {
        totalBytesRead += line.size();
    }

    // 1: Elapsed: 18.3 sec. Total read size: 8.33GiB. Average processing speed: 465MiB/s.

    // 2 (used std::move): Elapsed: 17.7 sec. Total read size: 8.33GiB. Average processing speed: 482MiB/s.

    // 3 (move + count quotes): Elapsed: 10.6 sec. Total read size: 8.33GiB. Average processing speed: 804MiB/s (up to 817MiB/s).

    auto finish = TInstant::Now();
    auto duration = finish - start;

    // NOTE: CLI import file csv: `Elapsed: 63.9 sec. Total read size: 8.33GiB. Average processing speed: 134MiB/s.`
    std::cerr << "Elapsed: " << std::setprecision(3) << duration.SecondsFloat() << " sec. Total read size: "
    << PrettifyBytes(totalBytesRead) << ". Average processing speed: "
    << PrettifyBytes((double)totalBytesRead / duration.SecondsFloat())  << "/s." << std::endl;

}


void testCsvSplitter(const TString& filePath) {
    const TFsPath dataFile(filePath);

    if (!dataFile.Exists() || !dataFile.IsFile()) {
        std::cerr << "Not a file or file doesn't exist: " << filePath << std::endl;
        exit(EXIT_FAILURE);
    }

    TFile file(filePath, OpenExisting | RdOnly | Seq);
    const i64 fileLength = file.GetLength();

    std::cout << "File length: " << fileLength << std::endl;

    const ui64 fileBufferSize = 2_MB;
    std::unique_ptr<TFileInput> fileInput = std::make_unique<TFileInput>(file, fileBufferSize);

    IInputStream& input = *fileInput;

    TCountingInput countInput(&input);
    NCsvFormat::TLinesSplitter linesSplitter(countInput);

    ui64 totalBytesRead = 0;
    ui64 batchBytes = 0;
    std::vector<TString> buffer;

    auto start = TInstant::Now();
    TDuration batchProcessingDuration;
    ui64 batches = 0;

    while (TString line = linesSplitter.ConsumeLine()) {
        if (line.empty()) {
            continue;
        }

        batchBytes += line.size();
        buffer.push_back(std::move(line));

        if (batchBytes < 1_MB) {
            continue;
        }

        auto batchStart = TInstant::Now();
        for (const auto& line : buffer) {
            NCsvFormat::CsvSplitter splitter(line, ',');
            do {
                [[maybe_unused]] TStringBuf nextField = splitter.Consume();
            } while (splitter.Step());
        }
        auto batchEnd = TInstant::Now();
        batchProcessingDuration += batchEnd - batchStart;
        ++batches;

        batchBytes = 0;
        buffer.clear();
    }

    auto finish = TInstant::Now();
    auto duration = finish - start;

    std::cerr << "Elapsed: " << std::setprecision(3) << duration.SecondsFloat() << " sec. Total read size: "
    << PrettifyBytes(totalBytesRead) << ". Average processing speed: "
    << PrettifyBytes((double)totalBytesRead / duration.SecondsFloat())  << "/s." << std::endl;


    std::cerr << "Average batch processing duration: " << std::setprecision(3) << batchProcessingDuration.SecondsFloat() / batches << " sec." << std::endl;
    std::cerr << "Batch processing duration total: " << std::setprecision(3) << batchProcessingDuration.SecondsFloat() << " sec." << std::endl;
    std::cerr << "Batches: " << batches << std::endl;
}

// test NCsvFormat::TLinesSplitter
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <csv_filepath>" << std::endl;
        return 1;
    }

    TString filePath = argv[1];
    // testTLinesSplitter(filePath);
    testCsvSplitter(filePath);

    return 0;
}
