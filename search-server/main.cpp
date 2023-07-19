#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <optional>
#include <functional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqEmStr(const StringContainer& strings)
{
    set<string> non_empty_strings;
    for (const string& str : strings)
    {
        /*
        if (
        {
            throw invalid_argument("что то не то в стоп словах");
        }
        */
        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

bool CheckQuery(const string& query) {
    for (int i = 1; i < static_cast<int>(query.size()); ++i)
    {
        if ((query[i] == '-' && (query[i - 1] == '-' || query[i + 1] == '-')) || (query[i] == ' ' && query[i - 1] == '-') || (query[query.size() - 1] == '-')) {
            return false;
        }
    }
    return true;
}



enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

auto compareDocuments = [](const Document& lhs, const Document& rhs)
{
    if (abs(lhs.relevance - rhs.relevance) < numeric_limits<double>::epsilon())
    {
        return lhs.rating > rhs.rating;
    }
    else
    {
        return lhs.relevance > rhs.relevance;
    }
};


class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqEmStr(stop_words)) 
    {
        for (string str : stop_words_)
        {
            if (!IsValidWord(str))
            {
                throw invalid_argument("что то не так со стоп словом");
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    int GetDocumentId(int id)
    {
        if (id > static_cast<int>(documents_.size()) || id < 0)
        {
            throw out_of_range("id меньше нуля или выходит за пределы");
            //return SearchServer::INVALID_DOCUMENT_ID;
        }
        if (documents_.count(id))
        {
            return id;
        }
        else
        {
            return INVALID_DOCUMENT_ID;
        }
    }


    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings)
    {
        if (document_id < 0 || documents_.count(document_id) || !IsValidWord(document))
        {
            throw invalid_argument("ошибка ошибка");
        }
        else
        {
            const vector<string> words = SplitIntoWordsNoStop(document);

            const double inv_word_count = 1.0 / words.size();

            if (words.empty())
            {
                documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
            }
            for (const string& word : words)
            {

                word_to_document_freqs_[word][document_id] += inv_word_count;
                documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

            }
            
        }
    }
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const
    {
        if (!CheckQuery(raw_query))
        {
            throw invalid_argument("ошибка ошибка");
        }
        else if (!IsValidWord(raw_query))
        {
            throw invalid_argument("ошибка ошибка");
        }
        else
        {
            const Query query = ParseQuery(raw_query);
            auto matched_documents = FindAllDocuments(query, document_predicate);

            sort(matched_documents.begin(), matched_documents.end(), compareDocuments);

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
            {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
        }
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const
    {
        if (!CheckQuery(raw_query))
        {
            throw invalid_argument("ошибка ошибка");
        }
        else if (!IsValidWord(raw_query))
        {
            throw invalid_argument("ошибка ошибка");
        }
        else
        {
            // Ваша реализация данного метода
            const Query query = ParseQuery(raw_query);
            auto matched_documents = FindAllDocuments(query, status);

            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! продолжить правки с этого места
            sort(matched_documents.begin(), matched_documents.end(), compareDocuments);

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
            {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
        }

    }


    vector<Document>FindTopDocuments(const string& raw_query) const
    {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }


    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const
    {
        if (!IsValidWord(raw_query))
        {
            throw invalid_argument("ошибка ошибка");
        }
        if (!CheckQuery(raw_query))
        {
            throw invalid_argument("ошибка ошибка");
        }


        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;

        for (const string& word : query.plus_words)
        {

            if (word.substr(0, 2) == "--")
            {
                throw invalid_argument("ошибка ошибка");
            }

            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words)
        {

            if (word.substr(0, 2) == "--")
            {
                throw invalid_argument("ошибка ошибка");
            }
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }

        return make_tuple(matched_words, documents_.at(document_id).status);
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }


    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }



    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }

    vector<Document> FindAllDocuments(const Query& query, DocumentStatus status) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (document_data.status == status)
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }



};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    setlocale(LC_ALL, "RUS");

    
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова

    //vector<Document> documents;

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    (void)search_server.AddDocument(1, "я иду к тебе"s, DocumentStatus::ACTUAL, { 7, 2, 7 });



    cout << endl;
    search_server.FindTopDocuments("на твоем берегу"s, DocumentStatus::ACTUAL);

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    
    cout << endl;
    try
    {
        search_server.FindTopDocuments("на --твоем  ----- берегу"s, DocumentStatus::ACTUAL);
    }
    catch (const invalid_argument& inv)
    {
        cout << inv.what() << endl;
    }
    

    
    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    cout << endl;
    try
    {
        search_server.FindTopDocuments("\x02"s, DocumentStatus::ACTUAL);
    }
    catch (const invalid_argument& inv)
    {
        cout << inv.what() << endl;
    }
    

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;
    



    
    cout << "тест1" << endl;
    try
    {
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    }
        //cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    catch (const invalid_argument& inv)
    {
        cout << inv.what() << endl;
    }

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    cout << endl;
    cout << "тест2" << endl;

   
        //cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    try
    {
        search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    }
    //cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    catch (const invalid_argument& inv)
    {
        cout << inv.what() << endl;
    }
    

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    cout << endl;
    cout << "тест3" << endl;
        try
        {
            search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        }
        //cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
        catch (const invalid_argument& inv)
        {
            cout << inv.what() << endl;
        }
    

        
/*
    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    cout << endl;
    cout << "тест4" << endl;
    if (search_server.FindTopDocuments("пушистый-  "s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка 'пушистый-  ' в поисковом запросе"s << endl;
    }

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;

    cout << endl;
    cout << "тест5" << endl;
    if (search_server.FindTopDocuments("привет пушистый\x12"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка в поисковом запросе Спецсимолы"s << endl;
    }

    cout << endl;
    cout << search_server.GetDocumentCount() << endl;
    cout << endl;


    cout << endl;
    cout << "тест6" << endl;
    if (search_server.FindTopDocuments("пушистый -"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка 'пушистый -' в поисковом запросе"s << endl;
    }

    cout << endl;
    cout << "тест7" << endl;
    if (search_server.FindTopDocuments("кот --пушистый"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка 'кот --пушистый' в поисковом запросе"s << endl;
    }



    cout << endl;
    cout << "тест8" << endl;
    if (search_server.FindTopDocuments("-пушистый кот"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "вначале минус слово в поисковом запросе"s << endl;
    }



    cout << endl;
    cout << "тест9" << endl;
    if (search_server.FindTopDocuments("- -пушистый"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка '- -пушистый' в поисковом запросе"s << endl;
    }

    cout << endl;
    cout << "тест10" << endl;
    if (search_server.FindTopDocuments("иван-чай"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Должно работать"s << endl;
    }

    cout << endl;
    cout << "тест11" << endl;
    if (search_server.FindTopDocuments("  --машина"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "  --машина"s << endl;
    }
    */
    
}
