//
// Created by wirewhiz on 4/14/22.
//

#include "timeline.h"
#include <stdexcept>

ScheduledBlock* Timeline::getTimeBlock(const std::string& name)
{
    for(auto& block : _blocks)
    {
        if(name == block.name())
            return &block;
    }
    return nullptr;
}

void Timeline::addTask(const std::string& name, std::function<void()> task, const std::string& timeBlock)
{
    ScheduledBlock* block = getTimeBlock(timeBlock);
    if(!block)
        throw std::runtime_error("Could not find time block: " + timeBlock);
    block->addTask(ScheduledTask(name, task));
}

void Timeline::addBlock(const std::string& name) { _blocks.emplace_back(name); }

void Timeline::addBlockBefore(const std::string& name, const std::string& before)
{
    auto parentBlock = _blocks.begin();
    if(parentBlock->name() == before)
        _blocks.push_front(ScheduledBlock(name));
    parentBlock++;

    while(parentBlock != _blocks.end())
    {
        if(parentBlock->name() == before)
        {
            parentBlock--;
            _blocks.insert(parentBlock, ScheduledBlock(name));
            return;
        }
        parentBlock++;
    }
    addBlock(name);
}

void Timeline::addBlockAfter(const std::string& name, const std::string& before)
{
    auto parentBlock = _blocks.begin();
    while(parentBlock != _blocks.end())
    {
        if(parentBlock->name() == before)
        {
            _blocks.insert(parentBlock, ScheduledBlock(name));
            return;
        }
        parentBlock++;
    }
    addBlock(name);
}

void Timeline::run()
{
    for(auto& block : _blocks)
        block.run();
}

ScheduledTask::ScheduledTask(const std::string& name, std::function<void()> task)
{
    _task = task;
    _name = name;
}

void ScheduledTask::run() { _task(); }

const std::string& ScheduledTask::name() const { return _name; }

ScheduledBlock::ScheduledBlock(const std::string& name) { _name = name; }

void ScheduledBlock::run()
{
    for(auto& task : _tasks)
    {
        task.run();
    }
}

const std::string& ScheduledBlock::name() const { return _name; }

void ScheduledBlock::addTask(const ScheduledTask& runnable) { _tasks.push_front(runnable); }
