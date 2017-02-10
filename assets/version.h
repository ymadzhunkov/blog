#pragma once
class Version {
public:
    Version();
    const char * const git_sha1;
    const char * const git_description;
    const char * const package;
    const char * const build_type;
};
