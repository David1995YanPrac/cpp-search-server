#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {

    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {

    int result = 0;
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
        else
        {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

double log1(int doc_count, map<string, map<int, double>> docs, string str1)
{
    double IDF1 = log(static_cast<double>(doc_count) / docs.at(str1).size());

    return IDF1;
}

struct Document
{
    int id;
    double relevance;
};



class SearchServer
{
public:
    double document_count_ = 0.;

    void SetStopWords(const string& text) {

        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document)
    {
        ++document_count_;

        const vector<string> words = SplitIntoWordsNoStop(document);

        double TF2 = 1. / words.size();

        for (string word : words)
        {
            new_documents_[word][document_id] += TF2;
        }


    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {

        const Query query_words = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs)
            {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    map<string, map<int, double>> new_documents_;

    set<string> stop_words_;

    struct Query
    {
        set<string> words2;
        set<string> minus2;
    };

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {

        vector<string> words;

        for (const string& word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const
    {
        Query query;

        set<string> minus_slov;

        for (string str1 : SplitIntoWords(text))
        {
                if (str1[0] == '-')
                {
                    str1.erase(0, 1);
                    minus_slov.insert(str1);
                    continue;
                }
                else if (!stop_words_.count(str1) > 0)
                {
                    if (!minus_slov.count(str1) > 0)
                    {
                        query.words2.insert(str1);
                    }
                    else
                    {
                        query.minus2.insert(str1);
                    }
                }
        }

        return query;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const
    {

        vector<Document> doc;

        map<int, double> document_to_relevance;

        for (string str1 : query_words.words2)
        {
            if (new_documents_.count(str1) == 0)
            {
                continue;
            }

            double IDF1 = log1(document_count_, new_documents_, str1);

            for (const auto& [id, TF] : new_documents_.at(str1))
            {
                document_to_relevance[id] = document_to_relevance[id] + TF * IDF1;
            }

        }

        for (string str1 : query_words.minus2)
        {
            if (new_documents_.count(str1) == 0)
            {
                continue;
            }

            for (const auto& [id, TF] : new_documents_.at(str1))
            {
                document_to_relevance.erase(id);
            }
        }

        for (const auto& [id, relevance] : document_to_relevance)
        {
            doc.push_back({ id, relevance });
        }

        return doc;
    }

};


SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << " }"s << endl;
    }
}