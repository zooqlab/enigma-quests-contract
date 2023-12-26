#include <eosio/eosio.hpp>

using namespace eosio;

CONTRACT questscreate : public contract {
public:
    using contract::contract;

    // Define the structure for the table rows
    TABLE Quest {
        uint64_t id;
        std::vector<std::string> nfts;
        std::vector<std::string> tokens;
        uint64_t wls;
        eosio::time_point end;
        std::string questname;
        name account;
        uint64_t communityId;
        std::string avatar;

        // Specify the primary key for the table
        uint64_t primary_key() const { return id; }
    };

    // Define a Multi-Index table with the structure defined above
    using quests_table = multi_index<"quests"_n, Quest>;
    // tasks is like {taskId: true, taskId: false, quiz: true}
    TABLE User {
        uint64_t scoreId;
        uint64_t questId;
        uint64_t communityId;
        uint64_t score;
        name account;
        bool subscription;

        uint64_t primary_key() const { return scoreId; }
    };

    using users_table = multi_index<"users"_n, User>;
    TABLE Community {
        uint64_t communityId;
        std::string communityName;
        uint64_t score;
        std::string avatar;
        name account;
        uint64_t followers;
        std::vector<uint64_t> questIds;
        std::vector<std::string> banners;

        uint64_t primary_key() const { return communityId; }
    };

    using communities_table = multi_index<"communities"_n, Community>;

    TABLE Tasks {
        uint64_t taskId;
        std::string type;
        std::string requirements;
        std::string taskName;
        uint64_t reward;
        eosio::time_point completedat;
        std::string description;
        uint64_t  timescompl;
        name account;
        uint64_t relatedquest;

        uint64_t primary_key() const { return taskId; }
    };

    using tasks_table = multi_index<"tasks"_n, Tasks>;

    ACTION createscore(const uint64_t& scoreId, const uint64_t& questId, const uint64_t& score, const name account)
        {   
            require_auth(account); 
            require_auth(_self);
            users_table users(_self, account.value);
            quests_table quests(_self, account.value);
            auto questexists = quests.find(questId);
            check(questexists == quests.end(), "Related Quest is not found");
            users.emplace(account, [&](auto& row) {
                row.scoreId = scoreId;
                row.questId = questId;
                row.score = score;
                row.account = account;
            });
        }
    
    ACTION createtask(const uint64_t& taskId, const std::string& type, const std::string& requirements, const std::string& taskName, const uint64_t& reward, const std::string& description, const name account, const uint64_t& relatedquest)
        {   
            require_auth(account);
            tasks_table tasks(_self, _self.value);
            auto existing_task = tasks.find(taskId);
            check(existing_task == tasks.end(), "Task with this ID already exists");
            tasks.emplace(account, [&](auto& row) {
                row.taskId = taskId;
                row.type = type;
                row.requirements = requirements;
                row.taskName = taskName;
                row.reward = reward;
                row.description = description;
                row.account = account;
                row.relatedquest = relatedquest;
            });

        }

    ACTION edittask(const uint64_t& taskId, const std::string& type, const std::string& requirements, const std::string& taskName, const uint64_t& reward, const std::string& description, const name account, const uint64_t& relatedquest)
        {
            require_auth(account);
            check(taskId != 0, "taskId needs to be present");
            tasks_table tasks(_self, _self.value);
            auto iterator = tasks.find(taskId);
            check(iterator != tasks.end(), "Record not found");
            tasks.modify(iterator, account, [&](auto& row) {
            row.type = type;
            row.requirements = requirements;
            row.taskName = taskName;
            row.reward = reward;
            row.description = description;
            row.account = account;
            row.relatedquest = relatedquest;
        });
        }

    ACTION submittask(const uint64_t& taskId, const uint64_t& timescompl, const eosio::time_point& completedat, const name account) {
        require_auth(account);
        require_auth(_self);
        check(taskId != 0, "taskId needs to be present");
        tasks_table tasks(_self, account.value);
    

        tasks.emplace(account, [&](auto& row) {
                row.taskId = taskId;
                row.completedat = completedat;
                row.timescompl = timescompl;
            });
    }

    ACTION deletetask(const uint64_t& taskId, const name account) {
        require_auth(account);
        tasks_table tasks(_self, _self.value);
        auto iterator = tasks.find(taskId);
        check(iterator != tasks.end(), "Record not found");
        tasks.erase(iterator);
    }

    ACTION createcommun(const uint64_t& communityId, const std::string& communityName, const std::string& avatar, const name& account, const std::vector<uint64_t>& questIds, const std::vector<std::string>& banners)
                    {
                        require_auth(account);
                        communities_table communities(_self, account.value);
                        communities.emplace(account, [&](auto& row) {
                            row.communityId = communityId;
                            row.communityName = communityName;
                            row.score = 0;
                            row.avatar = avatar;
                            row.account = account;
                            row.followers = 0;
                            row.questIds = questIds;
                            row.banners = banners;
                        });
                    }

    ACTION editcommun(const uint64_t& communityId, const std::string& communityName, const std::string& avatar, const name& account, const uint64_t& followers, const std::vector<uint64_t>& questIds, const std::vector<std::string>& banners)
                    {
                        require_auth(account);
                        communities_table communities(get_self(), account.value);
                        auto iterator = communities.find(communityId);
                        check(iterator != communities.end(), "Record not found");
                        communities.modify(iterator, account, [&](auto& row) {
                            row.communityId = communityId;
                            row.communityName = communityName;
                            row.avatar = avatar;
                            row.questIds = questIds;
                            row.banners = banners;
                        });
                    }

    ACTION subscribe(const uint64_t& communityId, name account, const uint64_t& scoreId)
    {
        require_auth(account); 
        users_table users(_self, _self.value);
        auto iterator = users.find(scoreId);
        check(iterator != users.end(), "Record not found");
        users.emplace(account, [&](auto& row) {
            row.communityId = communityId;
            row.account = account;
            row.scoreId = scoreId;
        });
    }

    // Action to insert a row into the table
    ACTION createquest(const uint64_t& id, const std::vector<std::string>& nfts, const std::vector<std::string>& tokens,
                   const uint64_t& wls, const eosio::time_point& end, const std::string& questname, const uint64_t& communityId, const name& account, const std::string& avatar) 
                   {
        //check if account entered are logined
        require_auth(account);
        communities_table communities(get_self(), account.value);
        quests_table quests(get_self(), account.value);
        auto iterator = communities.find(communityId);
        check(iterator != communities.end(), "Community not found");
        quests.emplace(account, [&](auto& row) {
            row.id = id;
            row.nfts = nfts;
            row.tokens = tokens;
            row.wls = wls;
            row.end = end;
            row.account = account;
            row.communityId = communityId;
            row.questname = questname;
            row.avatar = avatar;
        });
    }

    ACTION  editquest(const uint64_t& id, const std::vector<std::string>& nfts, const std::vector<std::string>& tokens,
                   const uint64_t& wls, const eosio::time_point& end, const name account, const std::string& questname, const std::string& avatar) 
                   {
        //check if account entered are logined
        require_auth(account);
        quests_table quests(get_self(), account.value);
        auto iterator = quests.find(id);
        check(iterator != quests.end(), "Record not found");

        quests.modify(iterator, account, [&](auto& row) {
            row.nfts = nfts;
            row.tokens = tokens;
            row.wls = wls;
            row.end = end;
            row.questname = questname;
            row.avatar = avatar;
        });
    }
};

// Macro to enable EOSIO contract ABI generation
EOSIO_DISPATCH(questscreate, (createtask)(createcommun)(createquest)(editquest)(edittask)(deletetask)(subscribe)(editcommun)(submittask))
