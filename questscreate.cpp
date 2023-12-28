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
        std::vector<uint64_t> tasks;
        time_point_sec end;
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
        std::vector<std::string> requirements;
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

    ACTION questaddtask( uint64_t taskId, name account, uint64_t relatedquest) {
        require_auth(account);
        tasks_table tasks(_self, _self.value);
        quests_table quests(_self, account.value);
        auto task = tasks.find(taskId);
        auto quest = quests.find(relatedquest);
        auto tasksCol = quest->tasks;
        auto isAlreadyInArr = std::find(tasksCol.begin(), tasksCol.end(), taskId);
        check(task != tasks.end(), "Related task is not found");
        check(quest != quests.end(), "Related quest is not found");
        check(isAlreadyInArr != tasksCol.end(), "Task is already in tasks array in this quest");
        quests.modify(quest, account, [&](auto& row) {
            row.tasks.push_back(taskId);
        });
    }

    ACTION questremtask(uint64_t taskId, name account, uint64_t relatedquest) {
        require_auth(account);
        tasks_table tasks(_self, _self.value);
        quests_table quests(_self, account.value);
        auto quest = quests.find(relatedquest);
        auto task = tasks.find(taskId);
        auto taskscol = quest->tasks;
        auto task_itr = std::find(taskscol.begin(), taskscol.end(), taskId);
        check(task_itr != taskscol.end(), "Task is not present in Tasks");
        taskscol.erase(task_itr);
        quests.modify(quest, account, [&](auto& row) {
            row.tasks = taskscol;
        });
    }


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
    
    ACTION createtask(const uint64_t& taskId, const std::string& type, const std::vector<std::string>& requirements, const std::string& taskName, const uint64_t& reward, const std::string& description, const name account, const uint64_t& relatedquest)
        {   
            require_auth(account);
            quests_table quests(_self, account.value);
            auto quest = quests.find(relatedquest);
            check(quest != quests.end(), "Cant find quest with entered questId");
            tasks_table tasks(_self, _self.value);
            auto existing_task = tasks.find(taskId);
            check(existing_task == tasks.end(), "Task with this ID already exists");
            tasks.emplace(account, [&](auto& row) {
                row.taskId = taskId;
                row.relatedquest = relatedquest;
                row.type = type;
                row.requirements = requirements;
                row.taskName = taskName;
                row.reward = reward;
                row.description = description;
                row.account = account;
            });
            if (relatedquest != 0) {
                action(
            permission_level{account, "active"_n},            
            get_self(),
            "questaddtask"_n,
            std::make_tuple(taskId, account, relatedquest)
        ).send();
            }

        }

    ACTION edittask(const uint64_t& taskId, const std::string& type, const std::vector<std::string>& requirements, const std::string& taskName, const uint64_t& reward, const std::string& description, const name account, const uint64_t& relatedquest)
        {
            require_auth(account);
            tasks_table tasks(_self, _self.value);
            quests_table quests(_self, account.value);
            auto quest = quests.find(relatedquest);
            auto iterator = tasks.find(taskId);
            auto prevrelatedquest = iterator->relatedquest;
            name questCreator = quest->account;
            check(taskId != 0, "taskId needs to be present");
            check(questCreator == account, "You can add tasks to only your quests");
            check(quest != quests.end(), "Cant find quest with entered questId");            
            check(iterator != tasks.end(), "Record not found");
            tasks.modify(iterator, account, [&](auto& row) {
            row.type = type;
            row.requirements = requirements;
            row.taskName = taskName;
            row.reward = reward;
            row.description = description;
            row.relatedquest = relatedquest;
            });
            if (prevrelatedquest != relatedquest) {
                action(
                    permission_level{account, "active"_n},
                    _self,
                    "questremtask"_n,
                    std::make_tuple(taskId, account, prevrelatedquest)
                    ).send();
                if (relatedquest != 0) {
                    action(
                        permission_level{account, "active"_n},
                        _self,
                        "questaddtask"_n,
                        std::make_tuple(taskId, account, relatedquest)
                        ).send();
                    }
            }

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

    ACTION deletetask(const uint64_t& taskId, const name account, const uint64_t& relatedquest) {
        require_auth(account);
        tasks_table tasks(_self, _self.value);
        auto iterator = tasks.find(taskId);
        check(iterator != tasks.end(), "Record not found");
        tasks.erase(iterator);
        action(
            permission_level{account, "active"_n},
            _self,
            "questremtask"_n,
            std::make_tuple(taskId, account, relatedquest)
        ).send();
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

    ACTION editcommun(const uint64_t& communityId, const std::string& communityName, const std::string& avatar, const name& account, const std::vector<uint64_t>& questIds, const std::vector<std::string>& banners)
                    {
                        require_auth(account);
                        communities_table communities(get_self(), account.value);
                        auto iterator = communities.find(communityId);
                        auto oldowner = iterator->account;
                        check(oldowner == account, "You cant edit communinty created by other person.");
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
        communities_table communities(get_self(), account.value);
        auto commiterator = communities.find(communityId);
        auto iterator = users.find(scoreId);
        check(iterator != users.end(), "Record not found");
        users.emplace(account, [&](auto& row) {
            row.communityId = communityId;
            row.account = account;
            row.scoreId = scoreId;
        });
        communities.modify(commiterator, account, [&](auto& row) {
                            row.followers += 1;
                        });
    }

    // Action to insert a row into the table
    ACTION createquest(const uint64_t& id, const std::vector<std::string>& nfts, const std::vector<std::string>& tokens,
                   const uint64_t& wls, const time_point_sec& end, const std::string& questname, const uint64_t& communityId, const name& account, const std::string& avatar) 
                   {
        //check if account entered are logined
        require_auth(account);
        communities_table communities(get_self(), account.value);
        quests_table quests(get_self(), account.value);
        if (communityId != 0) {
            auto iterator = communities.find(communityId);
            name commcreator = iterator->account;
            check(commcreator == account, "You cant add your quest to not your community");
            check(iterator != communities.end(), "Community not found");
        }
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
                   const uint64_t& wls, const time_point_sec& end, const uint64_t communityId, const name account, const std::string& questname, const std::string& avatar) 
                   {
        //check if account entered are logined
        require_auth(account);
        communities_table communities(get_self(), account.value);
        quests_table quests(get_self(), account.value);
        auto iterator = quests.find(id);
        check(iterator != quests.end(), "Record not found");
        if (communityId != 0 ) {
            auto commiterator = communities.find(communityId);
            name commcreator = commiterator->account;
            check(commcreator == account, "You cant add your quest to not your community");
            check(commiterator != communities.end(), "Community not found");
        }
        quests.modify(iterator, account, [&](auto& row) {
            row.nfts = nfts;
            row.communityId = communityId;
            row.tokens = tokens;
            row.wls = wls;
            row.end = end;
            row.questname = questname;
            row.avatar = avatar;
        });
    }
};

// Macro to enable EOSIO contract ABI generation
EOSIO_DISPATCH(questscreate, (createtask)(createcommun)(createquest)(editquest)(edittask)(deletetask)(subscribe)(editcommun)(submittask)(questaddtask)(questremtask))
