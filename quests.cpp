#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;

CONTRACT enigmatest13 : public contract {
public:
    using contract::contract;

    // Define the structure for the table rows
    TABLE Quest {
        uint64_t questId;
        std::vector<uint64_t> tasks;
        uint64_t end;
        std::string questName;
        name account;
        uint64_t communityId;
        std::string avatar;

        // Specify the primary key for the table
        uint64_t primary_key() const { return questId; }
    };

    // Define a Multi-Index table with the structure defined above
    using quests_table = multi_index<"quests"_n, Quest>;
    
    TABLE User {
        uint64_t scoreId;
        uint64_t score;
        uint64_t communityId;
        name account;
        bool subscription;

        uint64_t primary_key() const { return scoreId; }
    };

    using users_table = multi_index<"users"_n, User>;
    TABLE Community {
        uint64_t communityId;
        std::string communityName;
        std::vector<uint64_t> nfts;
        asset tokens;
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
        uint64_t completedat;
        std::string description;
        uint64_t  timescompl;
        name account;
        uint64_t relatedquest;

        uint64_t primary_key() const { return taskId; }
    };

    using tasks_table = multi_index<"tasks"_n, Tasks>;

    ACTION questaddtask( uint64_t taskId, name account, uint64_t relatedquest) {
        require_auth(account);
        require_auth(_self);
        tasks_table tasks(_self, _self.value);
        quests_table quests(_self, account.value);
        auto task = tasks.find(taskId);
        check(task != tasks.end(), "Related task is not found");
        auto prevrelatedquestid = task->relatedquest;
        if (prevrelatedquestid != 0) {
            auto oldquest = quests.find(prevrelatedquestid);
            auto oldtasksCol = oldquest->tasks;
            auto task_itr = std::find(oldtasksCol.begin(), oldtasksCol.end(), taskId);
            if (task_itr != oldtasksCol.end()) {
                oldtasksCol.erase(task_itr);
                quests.modify(oldquest, account, [&](auto& row) {
                row.tasks = oldtasksCol;
                });
            }
            
        }
        auto quest = quests.find(relatedquest);
        auto tasksCol = quest->tasks;
        auto isAlreadyInArr = std::find(tasksCol.begin(), tasksCol.end(), taskId);
        check(quest != quests.end(), "Related quest is not found");
        check(isAlreadyInArr == tasksCol.end(), "Task is already in tasks array in this quest");
        quests.modify(quest, account, [&](auto& row) {
            row.tasks.push_back(taskId);
        });
        tasks.modify(task, account, [&](auto& row) {
            row.relatedquest = relatedquest;
        });
    }

    ACTION questremtask(uint64_t taskId, name account, uint64_t relatedquest) {
        require_auth(account);
        require_auth(_self);
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
        tasks.modify(task, account, [&](auto& row) {
            row.relatedquest = 0;
        });
    }

    
    ACTION createtask(const uint64_t& taskId, const std::string& type, const std::vector<std::string>& requirements, const std::string& taskName, const uint64_t& reward, const std::string& description, const name account)
        {   
            require_auth(account);
            check(std::to_string(taskId).length() == 16, "task id must be 16 digits long");
            quests_table quests(_self, account.value);
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
            });

        }

    ACTION edittask(const uint64_t& taskId, const std::string& taskName, const uint64_t& reward, const std::string& description, const name account)
        {
            require_auth(account);
            tasks_table tasks(_self, _self.value);
            auto iterator = tasks.find(taskId);
            check(taskId != 0, "taskId needs to be present");
            check(iterator != tasks.end(), "Record not found");
            tasks.modify(iterator, account, [&](auto& row) {
            row.timescompl = 0;
            row.taskName = taskName;
            row.reward = reward;
            row.description = description;
            });
        }

    ACTION submittask(const uint64_t& taskId, const name account) {
        require_auth(account);
        require_auth(_self);
        check(taskId != 0, "taskId needs to be present");
        tasks_table tasks(_self, account.value);
        // in user scope, we store user 
        tasks_table tasksContract(_self, account.value);
        users_table userScores(_self, account.value);
        auto taskInfo = tasksContract.find(taskId);
        check(taskInfo != tasksContract.end(), "Task is not found");
        auto reward = taskInfo->reward;
        auto relatedquest = taskInfo->relatedquest;
        check(relatedquest != 0, "You cant submit completion of task, that is not tied to any quest.");
        auto taskreport = tasks.find(taskId);
        auto relatedquestScore = userScores.find(relatedquest);
        // if user didnt has task report about this task create one, if he has, update it
        if (taskreport == tasks.end()) {
            tasks.emplace(account, [&](auto& row) {
                row.taskId = taskId;
                row.completedat = eosio::current_time_point().sec_since_epoch();
                row.timescompl = 1;
            });
            // if user is not created score for this related quest, add score, if created just append his score
            if (relatedquestScore == userScores.end()) {
                userScores.emplace(account, [&](auto& row) {
                    row.scoreId = relatedquest;
                    row.account = account;
                    row.score = reward;
                    row.subscription = false;
                });
            } else {
                auto existingScore = relatedquestScore->score;
                userScores.modify(relatedquestScore, account, [&](auto& row) {
                    row.score = existingScore + reward;
                });
            }
        } else {
            auto timescompletedbefore = taskreport->timescompl;
            tasks.modify(taskreport, account, [&](auto& row) {
                row.timescompl = timescompletedbefore + 1;
            });
            if (relatedquestScore == userScores.end()) {
                userScores.emplace(account, [&](auto& row) {
                    row.scoreId = relatedquest;
                    row.account = account;
                    row.score = reward;
                    row.subscription = false;
                });
            } else {
                auto existingScore = relatedquestScore->score;
                userScores.modify(relatedquestScore, account, [&](auto& row) {
                    row.score = existingScore + reward;
                });
            }
        }
    }

    ACTION deletetask(const uint64_t& taskId, const name account, const uint64_t& relatedquest) {
        require_auth(account);
        tasks_table tasks(_self, _self.value);
        auto iterator = tasks.find(taskId);
        check(iterator != tasks.end(), "Record not found");
        tasks.erase(iterator);
        action(
            permission_level{_self, "active"_n},
            _self,
            "questremtask"_n,
            std::make_tuple(taskId, account, relatedquest)
        ).send();
    }


    

    ACTION createcommun(const uint64_t& communityId, const std::string& communityName, const std::string& avatar, const name& account, const std::vector<std::string>& banners)
                    {
                        require_auth(account);
                        require_auth(_self);
                        check(std::to_string(communityId).length() == 16, "community id must be 16 digits long");
                        communities_table communities(_self, account.value);
                        communities.emplace(account, [&](auto& row) {
                            row.communityId = communityId;
                            row.communityName = communityName;
                            row.score = 0;
                            row.avatar = avatar;
                            row.account = account;
                            row.followers = 0;
                            row.banners = banners;
                        });
                    }

    ACTION editcommun(const uint64_t& communityId, const std::string& communityName, const std::string& avatar, const name& account, const std::vector<std::string>& banners)
                    {
                        require_auth(account);
                        require_auth(_self);
                        communities_table communities(get_self(), account.value);
                        auto iterator = communities.find(communityId);
                        auto oldowner = iterator->account;
                        check(oldowner == account, "You cant edit community created by other person.");
                        check(iterator != communities.end(), "Record not found");
                        communities.modify(iterator, account, [&](auto& row) {
                            row.communityName = communityName;
                            row.avatar = avatar;
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


    ACTION createquest(const uint64_t& questId, uint64_t end, const std::string& questName, const uint64_t& communityId, const name& account, const std::string& avatar) 
                   {
        require_auth(account);
        require_auth(_self);
        check(end >= (eosio::current_time_point().sec_since_epoch() + 24*60*60),"Entered date of quest End is either not a number or its duration is less than 24 hours");
        check(std::to_string(questId).length() == 16, "questId must be 16 digits long");
        communities_table communities(get_self(), account.value);
        quests_table quests(get_self(), account.value);
        if (communityId != 0) {
            auto iterator = communities.find(communityId);
            name commcreator = iterator->account;
            check(commcreator == account, "You cant add your quest to not your community");
            check(iterator != communities.end(), "Community not found");
        }
        quests.emplace(account, [&](auto& row) {
            row.questId = questId;
            row.end = end;
            row.account = account;
            row.communityId = communityId;
            row.questName = questName;
            row.avatar = avatar;
        });
    }

    ACTION  editquest(const uint64_t& questId, const uint64_t end, const uint64_t communityId, const name account, const std::string& questName, const std::string& avatar) 
                   {
        require_auth(account);
        require_auth(_self);
        check(end >= (eosio::current_time_point().sec_since_epoch() + 24*60*60),"Entered date of quest End is either not a number or its duration is less than 24 hours");
        communities_table communities(get_self(), account.value);
        quests_table quests(get_self(), account.value);
        auto iterator = quests.find(questId);
        check(iterator != quests.end(), "Record not found");
        if (communityId != 0 ) {
            auto commiterator = communities.find(communityId);
            name commcreator = commiterator->account;
            check(commcreator == account, "You cant add your quest to not your community");
            check(commiterator != communities.end(), "Community not found");
        }
        quests.modify(iterator, account, [&](auto& row) {
            row.communityId = communityId;
            row.end = end;
            row.questName = questName;
            row.avatar = avatar;
        });
    }

    [[eosio::on_notify("atomicassets::transfer")]]
    void nft_transfer(name from, name to, std::vector<uint64_t>& asset_ids, std::string memo)
        {
            check(memo.length() == 16, "In memo you need to specify communityId to which you want to allocate tokens. It must be 16 digits long");
            for (char c : memo) {
                check(std::isdigit(c), "Invalid character in a string, only digits allowed");
            }
            uint64_t communityId = std::stoull(memo);
            communities_table communities(get_self(), from.value);
            auto commun = communities.find(communityId);
            name commauthor = commun->account;
            check(commun != communities.end(), "Community with such id not exists");
            check(commauthor == from, "You need to be a community owner in order to add nfts to its storage");
            auto existarray = commun->nfts;
            existarray.reserve(existarray.size() + asset_ids.size());
            existarray.insert(existarray.end(), asset_ids.begin(), asset_ids.end());
            communities.modify(commun, _self, [&](auto& row) {
                row.nfts = existarray;
            });
        }

    [[eosio::on_notify("eosio.token::transfer")]]
    void on_transfer(name from, name to, asset quantity, std::string memo){
            check(memo.length() == 16, "In memo you need to specify communityId to which you want to allocate tokens. It must be 16 digits long");
            for (char c : memo) {
                check(std::isdigit(c), "Invalid character in a string, only digits allowed");
            }
            uint64_t communityId = std::stoull(memo);
            communities_table communities(get_self(), from.value);
            auto commun = communities.find(communityId);
            name commauthor = commun->account;
            check(commun != communities.end(), "Community with such id not exists");
            check(commauthor == from, "You need to be a community owner in order to add tokens to its storage");
            communities.modify(commun, from, [&](auto& row) {
                row.tokens = quantity;
            });
        }
     	
};
