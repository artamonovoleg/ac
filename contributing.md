# how to contribute

one of the easiest ways to contribute is to participate in discussions and discuss issues. you can also contribute by submitting pull requests with code changes.

## filing issues
the best way to get your bug fixed is to be as detailed as you can be about the problem.
providing a minimal project with steps to reproduce the problem is ideal.
here are questions you can answer before you file a bug to make sure you're not missing any important information.

1. did you read the documentation?
2. did you include the snippet of broken code in the issue?
3. what are the *EXACT* steps to reproduce this problem?
4. what version are you using?

when filing bugs make sure you check the markdown formatting before clicking submit.

## contributing code and content

make sure you can build the code. familiarize yourself with the project workflow and our coding conventions.

here's a few things you should always do when making changes to the code base:

**engineering guidelines**

the coding style, and general engineering guidelines follow those described in the docs/coding_standarts.md

ac has adopted a clang-format requirement for all incoming changes. PRs to ac should have the _changed code_ clang formatted to the ac clang-format style, and leave the remaining portions of the file unchanged. this can be done using the `git-clang-format` tool or IDE driven workflows.

**commit/pull request format**

```
summary of the changes (Less than 80 chars)
 - detail 1
 - detail 2

fixes #bugnumber (in this specific format)
```

your pull request should:

* include a description of what your change intends to do
  * the title of your PR should be a very brief description of the change, and
    can use tags to allow for speedy categorization.
    * titles under 76 characters print nicely in unix terminals under `git log`.
      this is not a hard requirement, but is good guidance.
    * title tags in use are:
      * [ common ] - change that affects all platforms,
        if you changed file without platform prefix you can use this
      * module specific tags - [ fs ], [ window ], [ renderer ], etc...
        same as common but more specfic, prefer this over common
      * platform specific tags
        * [ windows ]
        * [ linux ]
        * [ apple ]
      * for renderer related changes
        * [ d3d12 ]
        * [ vulkan ]
        * [ metal ]
      * [ premake ] - build scripts related changes
      * [ docs ] - documentation change
      * tags are limited to set listed above. please don't use other tags because it will make harder to sort changes related to specific code part
  * the PR description should include a more detailed description of the change,
    an explanation for the motivation of the change, and links to any relevant
    issues. this does not need to be a dissertation, but should leave
    breadcrumbs for the next person debugging your code (who might be you).
* please dont't use capitals in descriptions, commit messages, comments or any other places. this is silly requirement but this is part of codestyle
* be a child commit of a reasonably recent commit in the main branch
* pass all unit tests
* ensure that the title and description are fully up to date before merging
  * the title and description feed the final git commit message, and we want to
    ensure high quality commit messages in the repository history.
* if this is new feature include adequate tests in ac-examples repo
