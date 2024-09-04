void MessageHandler::init_index_file(const std::string &idx_path) {
    //索引文件存在，则读取索引内容
    auto file_size = std::filesystem::file_size(idx_path);
    size_t txn_count = file_size / sizeof(TxnEntry);
    size_t valid_size = txn_count * sizeof(TxnEntry);
    if (valid_size != file_size) {
        fmt::println("Warning: File size ({}) is not a multiple of TxnEntry size ({}). Possible file corruption. Using only the {} txns.", file_size, sizeof(TxnEntry), txn_count);
    }
    std::vector<TxnEntry> txn_entries(txn_count);
    // 读取文件到vector中
    std::ifstream file(idx_path, std::ios::binary);
    if (!file.read(reinterpret_cast<char *>(txn_entries.data()), txn_count * sizeof(TxnEntry))) {
        throw std::runtime_error("Failed to read the txn entrys.");
    }
    processed_txn_id_ = txn_entries.back().txnId;
    fmt::println("last processed txnid is {}", processed_txn_id_);

    //前缀和，便于查询
    data_count_prefix_sum_[0] = txn_entries[0].dataCnt;
    for (int i = 1; i < txn_entries.size(); i++) {
        //前缀和计算的同时校验data_sq是否正确
        data_count_prefix_sum_[i] = data_count_prefix_sum_[i - 1] + txn_entries[i].dataCnt;
        if (data_count_prefix_sum_[i] != txn_entries[i].lstDataSqno) {
            throw std::runtime_error("error!");
        }
    }
}

void MessageHandler::verify_log_file(const std::string &log_file) {
    //根据索引文件的读取结果，对log文件进行裁剪和校验
    //根据data_count 已直到buffer size 判断
    auto file_size = std::filesystem::file_size(log_file);
    auto expected_size = data_count_prefix_sum_.back() * DspMessage::buffer_size;
    if (file_size < expected_size) {
        throw std::runtime_error("log file size is less than expected, Possible file corruption");
    } else if (file_size > expected_size) {
        fmt::println("log file size is largeer than expected, log file need tuncated");
    } else {
        fmt::println("The log file size exactly matches index file expected .");
    }
    content_ = std::make_shared<EntryFile>(log_file, expected_size);
}