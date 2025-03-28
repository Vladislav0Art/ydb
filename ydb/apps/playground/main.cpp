
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


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <csv_filepath>" << std::endl;
        return 1;
    }

    TString filePath = argv[1];
    const TFsPath dataFile(filePath);

    if (!dataFile.Exists() || !dataFile.IsFile()) {
        std::cerr << "Not a file or file doesn't exist: " << filePath << std::endl;
        return EXIT_FAILURE;
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
    // ui64 printed = 0;

    auto start = TInstant::Now();

    while (TString line = splitter.ConsumeLine()) {
        totalBytesRead += line.size();
        // if (printed < 10) {
        //     std::cout << line << std::endl;
        //     ++printed;
        // }
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

    return 0;
}

/*
event_time,event_type,product_id,category_id,category_code,brand,price,user_id,user_session
2019-11-01 00:00:00 UTC,view,1003461,2053013555631882655,electronics.smartphone,xiaomi,489.07,520088904,4d3b30da-a5e4-49df-b1a8-ba5943f1dd33
2019-11-01 00:00:00 UTC,view,5000088,2053013566100866035,appliances.sewing_machine,janome,293.65,530496790,8e5f4f83-366c-4f70-860e-ca7417414283
2019-11-01 00:00:01 UTC,view,17302664,2053013553853497655,,creed,28.31,561587266,755422e7-9040-477b-9bd2-6a6e8fd97387
2019-11-01 00:00:01 UTC,view,3601530,2053013563810775923,appliances.kitchen.washer,lg,712.87,518085591,3bfb58cd-7892-48cc-8020-2f17e6de6e7f
2019-11-01 00:00:01 UTC,view,1004775,2053013555631882655,electronics.smartphone,xiaomi,183.27,558856683,313628f1-68b8-460d-84f6-cec7a8796ef2
2019-11-01 00:00:01 UTC,view,1306894,2053013558920217191,computers.notebook,hp,360.09,520772685,816a59f3-f5ae-4ccd-9b23-82aa8c23d33c
2019-11-01 00:00:01 UTC,view,1306421,2053013558920217191,computers.notebook,hp,514.56,514028527,df8184cc-3694-4549-8c8c-6b5171877376
2019-11-01 00:00:02 UTC,view,15900065,2053013558190408249,,rondell,30.86,518574284,5e6ef132-4d7c-4730-8c7f-85aa4082588f
2019-11-01 00:00:02 UTC,view,12708937,2053013553559896355,,michelin,72.72,532364121,0a899268-31eb-46de-898d-09b2da950b24
*/