#include <NeForce/core/file/mermaid/mermaid.hpp>
#include <NeForce/core/string/width.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    bool contains(const mermaid_art& art, const string_view needle) {
        for (const string& line: art.lines) {
            if (line.view().find(needle) != string_view::npos) {
                return true;
            }
        }
        return false;
    }

    size_t count_text(const mermaid_art& art, const string_view needle) {
        if (needle.empty()) {
            return 0;
        }
        size_t count = 0;
        for (const string& line: art.lines) {
            size_t position = 0;
            while (position < line.size()) {
                const size_t found = line.view().find(needle, position);
                if (found == string_view::npos) {
                    break;
                }
                ++count;
                position = found + needle.size();
            }
        }
        return count;
    }

    bool has_role(const mermaid_art& art, const mermaid_role role) {
        for (const vector<mermaid_run>& runs: art.styled) {
            for (const mermaid_run& run: runs) {
                if (run.role == role) {
                    return true;
                }
            }
        }
        return false;
    }

    string role_text(const mermaid_art& art, const mermaid_role role) {
        string result;
        for (size_t i = 0; i < art.styled.size(); ++i) {
            for (const mermaid_run& run: art.styled[i]) {
                if (run.role == role) {
                    result.append(art.lines[i].data() + run.begin, run.end - run.begin);
                }
            }
        }
        return result;
    }

    bool widths_match(const mermaid_art& art) {
        int widest = 0;
        for (const string& line: art.lines) {
            const int width = text_display_width(line.view());
            if (width != art.width) {
                return false;
            }
            widest = max(widest, width);
        }
        return widest == art.width;
    }

    bool runs_cover_lines(const mermaid_art& art) {
        if (art.lines.size() != art.styled.size()) {
            return false;
        }
        for (size_t i = 0; i < art.lines.size(); ++i) {
            size_t position = 0;
            string rebuilt;
            for (const mermaid_run& run: art.styled[i]) {
                if (run.begin != position || run.end < run.begin || run.end > art.lines[i].size()) {
                    return false;
                }
                rebuilt.append(art.lines[i].data() + run.begin, run.end - run.begin);
                position = run.end;
            }
            if (position != art.lines[i].size() || rebuilt != art.lines[i]) {
                return false;
            }
        }
        return true;
    }

    bool roles_are_valid(const mermaid_art& art) {
        for (const vector<mermaid_run>& runs: art.styled) {
            for (const mermaid_run& run: runs) {
                if (static_cast<uint8_t>(run.role) > static_cast<uint8_t>(mermaid_role::title)) {
                    return false;
                }
            }
        }
        return true;
    }

    void expect_invariants(const mermaid_art& art) {
        EXPECT_EQ(art.lines.size(), art.styled.size());
        EXPECT_TRUE(widths_match(art));
        EXPECT_TRUE(runs_cover_lines(art));
        EXPECT_TRUE(roles_are_valid(art));
        EXPECT_GT(art.width, 0);
    }
} // namespace

TEST(MermaidTest, FlowchartTopDownBordersAndArrows) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\n A[Start] --> B[Middle]\n B --> C[End]\n");
    ASSERT_TRUE(art.has_value());
    ASSERT_FALSE(art->lines.empty());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Start"));
    EXPECT_TRUE(contains(*art, "Middle"));
    EXPECT_TRUE(contains(*art, "End"));
    EXPECT_TRUE(contains(*art, "┌"));
    EXPECT_TRUE(contains(*art, "┘"));
    EXPECT_TRUE(contains(*art, "│"));
    EXPECT_GE(count_text(*art, "▼"), size_t(2));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
}

TEST(MermaidTest, FlowchartLeftRightDirection) {
    const optional<mermaid_art> art = render_mermaid("graph LR\n A[Start] --> B[Middle] --> C[End]\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Start"));
    EXPECT_TRUE(contains(*art, "End"));
    EXPECT_TRUE(contains(*art, "▶"));
    EXPECT_EQ(count_text(*art, "▼"), size_t(0));
    EXPECT_LE(art->lines.size(), size_t(5));
}

TEST(MermaidTest, FlowchartTbAndBodyDirection) {
    const optional<mermaid_art> top_down = render_mermaid("graph TB\n A --> B\n");
    ASSERT_TRUE(top_down.has_value());
    EXPECT_TRUE(contains(*top_down, "▼"));
    EXPECT_EQ(count_text(*top_down, "▶"), 0);

    const optional<mermaid_art> left_right = render_mermaid("flowchart TD\n direction LR\n A --> B\n");
    ASSERT_TRUE(left_right.has_value());
    EXPECT_TRUE(contains(*left_right, "▶"));
}

TEST(MermaidTest, FlowchartShapesAndEdgeLabels) {
    const optional<mermaid_art> art =
            render_mermaid("flowchart TD\n A(Round) -->|yes| B{Diamond}\n A -- maybe --> C((Circle))\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Round"));
    EXPECT_TRUE(contains(*art, "Diamond"));
    EXPECT_TRUE(contains(*art, "Circle"));
    EXPECT_TRUE(contains(*art, "╭"));
    EXPECT_TRUE(contains(*art, "╱"));
    EXPECT_TRUE(contains(*art, "yes"));
    EXPECT_TRUE(contains(*art, "maybe"));
    const string labels = role_text(*art, mermaid_role::edge_label);
    EXPECT_NE(labels.view().find("yes"), string_view::npos);
    EXPECT_NE(labels.view().find("maybe"), string_view::npos);
}

TEST(MermaidTest, FlowchartEdgeStyles) {
    const optional<mermaid_art> dotted = render_mermaid("flowchart TD\n A --> B\n B -.-> C\n");
    ASSERT_TRUE(dotted.has_value());
    EXPECT_TRUE(contains(*dotted, "┆"));
    EXPECT_GE(count_text(*dotted, "▼"), 2);

    const optional<mermaid_art> thick = render_mermaid("flowchart TD\n A ==> B\n");
    ASSERT_TRUE(thick.has_value());
    EXPECT_TRUE(contains(*thick, "┃") || contains(*thick, "┳") || contains(*thick, "┻"));

    const optional<mermaid_art> plain = render_mermaid("flowchart TD\n A --- B\n");
    ASSERT_TRUE(plain.has_value());
    EXPECT_EQ(count_text(*plain, "▼"), 0);
    EXPECT_TRUE(contains(*plain, "┴"));
}

TEST(MermaidTest, SequenceDiagramLifelinesAndArrows) {
    const optional<mermaid_art> art = render_mermaid("sequenceDiagram\n participant A as Alice\n participant B as "
                                                     "Bob\n A->>B: Hello\n B-->>A: Hi\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Alice"));
    EXPECT_TRUE(contains(*art, "Bob"));
    EXPECT_TRUE(contains(*art, "Hello"));
    EXPECT_TRUE(contains(*art, "Hi"));
    EXPECT_TRUE(contains(*art, "│"));
    EXPECT_TRUE(contains(*art, "▶"));
    EXPECT_TRUE(contains(*art, "◀"));
    EXPECT_TRUE(contains(*art, "·"));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge_label));
}

TEST(MermaidTest, SequenceDiagramNoteAndSelfMessage) {
    const optional<mermaid_art> art =
            render_mermaid("sequenceDiagram\n participant A as Alice\n A->>A: think\n Note over A: waiting\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "think"));
    EXPECT_TRUE(contains(*art, "Note: waiting"));
    EXPECT_TRUE(contains(*art, "◀"));
    EXPECT_TRUE(contains(*art, "Alice"));
}

TEST(MermaidTest, SequenceDiagramImplicitCjkParticipants) {
    const optional<mermaid_art> art = render_mermaid("sequenceDiagram\n 张三->>李四: 你好\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "张三"));
    EXPECT_TRUE(contains(*art, "李四"));
    EXPECT_TRUE(contains(*art, "你好"));
    EXPECT_GT(art->width, 10);
}

TEST(MermaidTest, StateDiagram) {
    const optional<mermaid_art> art =
            render_mermaid("stateDiagram-v2\n [*] --> Idle\n Idle --> Running : start\n Running --> [*]\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Idle"));
    EXPECT_TRUE(contains(*art, "Running"));
    EXPECT_TRUE(contains(*art, "●"));
    EXPECT_GE(count_text(*art, "▼"), size_t(3));
    EXPECT_TRUE(contains(*art, "start"));
    const string labels = role_text(*art, mermaid_role::edge_label);
    EXPECT_NE(labels.view().find("start"), string_view::npos);
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
}

TEST(MermaidTest, StateDiagramNamedState) {
    const optional<mermaid_art> art =
            render_mermaid("stateDiagram-v2\n state \"Long Name\" as S\n [*] --> S\n S --> T : go\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Long Name"));
    EXPECT_TRUE(contains(*art, "T"));
    EXPECT_TRUE(contains(*art, "go"));
}

TEST(MermaidTest, WidthEqualsWidestLine) {
    const char* sources[] = {
            "flowchart TD\n A[Start] --> B[End]\n",
            "flowchart LR\n A[Start] --> B[End]\n",
            "sequenceDiagram\n A->>B: Hello\n B-->>A: Hi\n",
            "stateDiagram-v2\n [*] --> A\n A --> B : go\n",
            "flowchart TD\n A[中文节点] --> B[End]\n",
            "---\ntitle: A Very Long Title\n---\nflowchart TD\n A --> B\n",
    };
    for (const char* source: sources) {
        const optional<mermaid_art> art = render_mermaid(source);
        ASSERT_TRUE(art.has_value());
        int widest = 0;
        for (const string& line: art->lines) {
            widest = max(widest, text_display_width(line.view()));
        }
        EXPECT_EQ(art->width, widest);
        for (const string& line: art->lines) {
            EXPECT_EQ(text_display_width(line.view()), art->width);
        }
    }
}

TEST(MermaidTest, StyledRunsReconstructEveryLine) {
    const char* sources[] = {
            "flowchart TD\n A[Start] -->|go| B[End]\n",
            "flowchart LR\n A --> B --> C\n",
            "sequenceDiagram\n participant A as Alice\n A->>A: loop\n Note over A: note\n",
            "stateDiagram-v2\n [*] --> A\n A --> B : go\n",
            "flowchart TD\n A[中文节点] --> B[End]\n",
    };
    for (const char* source: sources) {
        const optional<mermaid_art> art = render_mermaid(source);
        ASSERT_TRUE(art.has_value());
        EXPECT_EQ(art->lines.size(), art->styled.size());
        EXPECT_TRUE(runs_cover_lines(*art));
        EXPECT_TRUE(roles_are_valid(*art));
    }
}

TEST(MermaidTest, TitleRoleAndPadding) {
    const optional<mermaid_art> art = render_mermaid("---\ntitle: My Chart\n---\nflowchart TD\n A[Hi] --> B[Bye]\n");
    ASSERT_TRUE(art.has_value());
    ASSERT_FALSE(art->lines.empty());
    expect_invariants(*art);
    EXPECT_TRUE(art->lines[0].view().starts_with("My Chart"));
    bool found_title = false;
    for (const mermaid_run& run: art->styled[0]) {
        if (run.role == mermaid_role::title) {
            found_title = true;
        }
    }
    EXPECT_TRUE(found_title);
    EXPECT_TRUE(role_text(*art, mermaid_role::title).view() == string_view("My Chart"));
}

TEST(MermaidTest, BodyTitleLine) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\ntitle: Chart Name\nA --> B\n");
    ASSERT_TRUE(art.has_value());
    ASSERT_FALSE(art->lines.empty());
    expect_invariants(*art);
    EXPECT_TRUE(art->lines[0].view().starts_with("Chart Name"));
    EXPECT_TRUE(has_role(*art, mermaid_role::title));
}

TEST(MermaidTest, UnsupportedDiagramTypesReturnNone) {
    EXPECT_FALSE(render_mermaid("pie\n a : 1\n").has_value());
    EXPECT_FALSE(render_mermaid("gantt\n title A\n").has_value());
    EXPECT_FALSE(render_mermaid("classDiagram\n A <|-- B\n").has_value());
    EXPECT_FALSE(render_mermaid("not a diagram\n").has_value());
}

TEST(MermaidTest, OversizedSourceReturnsNone) {
    string source;
    source.append(20001, 'x');
    EXPECT_FALSE(render_mermaid(source.view()).has_value());
}

TEST(MermaidTest, EmptyAndHeaderOnlyInput) {
    EXPECT_FALSE(render_mermaid(string_view()).has_value());
    EXPECT_FALSE(render_mermaid("   \n\t\n").has_value());
    EXPECT_FALSE(render_mermaid("%% only a comment\n").has_value());
    EXPECT_FALSE(render_mermaid("flowchart TD\n").has_value());
    EXPECT_FALSE(render_mermaid("sequenceDiagram\n").has_value());
    EXPECT_FALSE(render_mermaid("stateDiagram-v2\n").has_value());
}

TEST(MermaidTest, MalformedLinesAreSkipped) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\n A -->\n D[unclosed\n B[ok] --> C[ok2]\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "ok"));
    EXPECT_TRUE(contains(*art, "ok2"));
    EXPECT_FALSE(contains(*art, "unclosed"));
}

TEST(MermaidTest, UnknownDirectivesAreSkipped) {
    const optional<mermaid_art> art = render_mermaid(
            "flowchart TD\n classDef red fill:#f00\n subgraph one\n A --> B\n end\n style A fill:#fff\n C --> A\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "A"));
    EXPECT_TRUE(contains(*art, "B"));
    EXPECT_TRUE(contains(*art, "C"));
    EXPECT_EQ(count_text(*art, "fill:#f00"), 0);
    EXPECT_EQ(count_text(*art, "subgraph"), 0);
}

TEST(MermaidTest, CyclesAreSafe) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\n A --> B\n B --> C\n C --> A\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "A"));
    EXPECT_TRUE(contains(*art, "B"));
    EXPECT_TRUE(contains(*art, "C"));
    EXPECT_LT(art->lines.size(), size_t(100));
}

TEST(MermaidTest, TwoNodeCycleUsesUpArrow) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\n A --> B\n B --> A\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "▲"));
    EXPECT_TRUE(contains(*art, "▼"));
    EXPECT_TRUE(contains(*art, "A"));
    EXPECT_TRUE(contains(*art, "B"));
}

TEST(MermaidTest, CjkFlowchartKeepsColumnAlignment) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\n A[中文节点] --> B[End]\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "中文节点"));
    EXPECT_TRUE(contains(*art, "┌"));
    EXPECT_TRUE(contains(*art, "┐"));
}

TEST(MermaidTest, SameRankEdgeIsDrawn) {
    const optional<mermaid_art> art = render_mermaid("flowchart TD\n A --> B\n A --> C\n B --> C\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "├"));
    EXPECT_TRUE(contains(*art, "▶"));
    EXPECT_TRUE(contains(*art, "B"));
    EXPECT_TRUE(contains(*art, "C"));
}

TEST(MermaidTest, SequenceDiagramLeftwardMessageUsesLeftArrow) {
    const optional<mermaid_art> art = render_mermaid("sequenceDiagram\n A->>B: right\n B->>A: left\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "▶"));
    EXPECT_TRUE(contains(*art, "◀"));
    EXPECT_GE(count_text(*art, "▶"), size_t(1));
    EXPECT_GE(count_text(*art, "◀"), size_t(1));
}

TEST(MermaidTest, SequenceDiagramCrossingKeepsLifelines) {
    const optional<mermaid_art> art =
            render_mermaid("sequenceDiagram\n participant A\n participant B\n participant C\n A->>C: across\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "across"));
    EXPECT_GE(count_text(*art, "┼"), size_t(1));
}

TEST(MermaidTest, DeterministicOutput) {
    const char* source = "flowchart TD\n A[Start] --> B{Choice}\n B --> C[End]\n";
    const optional<mermaid_art> first = render_mermaid(source);
    const optional<mermaid_art> second = render_mermaid(source);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(first->lines.size(), second->lines.size());
    ASSERT_EQ(first->styled.size(), second->styled.size());
    for (size_t i = 0; i < first->lines.size(); ++i) {
        EXPECT_EQ(first->lines[i], second->lines[i]);
        ASSERT_EQ(first->styled[i].size(), second->styled[i].size());
        for (size_t j = 0; j < first->styled[i].size(); ++j) {
            EXPECT_EQ(first->styled[i][j].begin, second->styled[i][j].begin);
            EXPECT_EQ(first->styled[i][j].end, second->styled[i][j].end);
            EXPECT_EQ(first->styled[i][j].role, second->styled[i][j].role);
        }
    }
}

TEST(MermaidTest, LinesArePaddedToBlockWidth) {
    const optional<mermaid_art> art =
            render_mermaid("---\ntitle: A Title Wider Than The Graph\n---\nflowchart TD\n A --> B\n");
    ASSERT_TRUE(art.has_value());
    ASSERT_GE(art->lines.size(), size_t(2));
    for (const string& line: art->lines) {
        EXPECT_EQ(text_display_width(line.view()), art->width);
    }
    EXPECT_EQ(art->lines[0].back(), 'h');
    EXPECT_EQ(art->lines[1].back(), ' ');
}

TEST(MermaidTest, ClassDiagramBoxesMembersAndRelations) {
    const optional<mermaid_art> art = render_mermaid("classDiagram\n"
                                                     " class Animal {\n"
                                                     "  +int age\n"
                                                     "  -String name\n"
                                                     " }\n"
                                                     " class Dog {\n"
                                                     "  +void bark()\n"
                                                     " }\n"
                                                     " class Owner\n"
                                                     " Animal <|-- Dog : inherits\n"
                                                     " Dog *-- Owner : owns\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Animal"));
    EXPECT_TRUE(contains(*art, "Dog"));
    EXPECT_TRUE(contains(*art, "Owner"));
    EXPECT_TRUE(contains(*art, "+int age"));
    EXPECT_TRUE(contains(*art, "-String name"));
    EXPECT_TRUE(contains(*art, "+void bark()"));
    EXPECT_TRUE(contains(*art, "inherits"));
    EXPECT_TRUE(contains(*art, "owns"));
    EXPECT_TRUE(contains(*art, "△"));
    EXPECT_TRUE(contains(*art, "◆"));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge_label));
}

TEST(MermaidTest, ClassDiagramSingleLineBodyAndDottedRelations) {
    const optional<mermaid_art> art = render_mermaid("classDiagram\n"
                                                     " class A { +int x; -void f() }\n"
                                                     " class B\n"
                                                     " class C\n"
                                                     " A ..> B : uses\n"
                                                     " B ..|> C : implements\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "+int x"));
    EXPECT_TRUE(contains(*art, "-void f()"));
    EXPECT_TRUE(contains(*art, "uses"));
    EXPECT_TRUE(contains(*art, "implements"));
    EXPECT_TRUE(contains(*art, "▽"));
    EXPECT_TRUE(contains(*art, "┆"));
    EXPECT_TRUE(contains(*art, "△"));
}

TEST(MermaidTest, ErDiagramEntitiesCardinalityAndLabel) {
    const optional<mermaid_art> art = render_mermaid("erDiagram\n"
                                                     " CUSTOMER {\n"
                                                     "  string name\n"
                                                     "  int id\n"
                                                     " }\n"
                                                     " ORDER {\n"
                                                     "  int id\n"
                                                     " }\n"
                                                     " CUSTOMER ||--o{ ORDER : places\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "CUSTOMER"));
    EXPECT_TRUE(contains(*art, "ORDER"));
    EXPECT_TRUE(contains(*art, "string name"));
    EXPECT_TRUE(contains(*art, "int id"));
    EXPECT_TRUE(contains(*art, "places"));
    EXPECT_TRUE(contains(*art, "1"));
    EXPECT_TRUE(contains(*art, "0..*"));
    EXPECT_TRUE(contains(*art, "▼"));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge_label));
}

TEST(MermaidTest, ClassDiagramBranchRelationsUseSideChannel) {
    const optional<mermaid_art> art = render_mermaid("classDiagram\n"
                                                     " class A\n"
                                                     " class B\n"
                                                     " class C\n"
                                                     " A <|-- B : left branch\n"
                                                     " A <|-- C : right branch\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "left branch"));
    EXPECT_TRUE(contains(*art, "right branch"));
    EXPECT_TRUE(contains(*art, "┐"));
    EXPECT_TRUE(contains(*art, "┘"));
    EXPECT_TRUE(contains(*art, "△"));
    EXPECT_TRUE(contains(*art, "B"));
    EXPECT_TRUE(contains(*art, "C"));
}

TEST(MermaidTest, ErDiagramImplicitEntities) {
    const optional<mermaid_art> art = render_mermaid("erDiagram\n CUSTOMER ||--o{ ORDER : places\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "CUSTOMER"));
    EXPECT_TRUE(contains(*art, "ORDER"));
    EXPECT_TRUE(contains(*art, "places"));
    EXPECT_TRUE(contains(*art, "0..*"));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
}

TEST(MermaidTest, ErDiagramBranchRelationsUseSideChannel) {
    const optional<mermaid_art> art = render_mermaid("erDiagram\n"
                                                     " CUSTOMER ||--o{ ORDER : places\n"
                                                     " CUSTOMER ||--o{ ADDRESS : has\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "CUSTOMER"));
    EXPECT_TRUE(contains(*art, "ORDER"));
    EXPECT_TRUE(contains(*art, "ADDRESS"));
    EXPECT_TRUE(contains(*art, "places"));
    EXPECT_TRUE(contains(*art, "has"));
    EXPECT_TRUE(contains(*art, "◀"));
    EXPECT_TRUE(contains(*art, "0..*"));
}

TEST(MermaidTest, ErDiagramSingleLineEntityBlocks) {
    const optional<mermaid_art> art = render_mermaid("erDiagram\n"
                                                     " CUSTOMER { string name }\n"
                                                     " ORDER { int id }\n"
                                                     " CUSTOMER ||--o{ ORDER : places\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "CUSTOMER"));
    EXPECT_TRUE(contains(*art, "ORDER"));
    EXPECT_TRUE(contains(*art, "string name"));
    EXPECT_TRUE(contains(*art, "int id"));
    EXPECT_TRUE(contains(*art, "places"));
    EXPECT_TRUE(contains(*art, "0..*"));
}

TEST(MermaidTest, GitGraphCommitsBranchesAndMerge) {
    const optional<mermaid_art> art = render_mermaid("gitGraph\n"
                                                     " commit id: \"abc\"\n"
                                                     " branch feature\n"
                                                     " commit id: \"def\"\n"
                                                     " checkout main\n"
                                                     " commit id: \"ghi\"\n"
                                                     " merge feature id: \"m1\"\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_GE(count_text(*art, "●"), size_t(4));
    EXPECT_TRUE(contains(*art, "abc"));
    EXPECT_TRUE(contains(*art, "def"));
    EXPECT_TRUE(contains(*art, "ghi"));
    EXPECT_TRUE(contains(*art, "m1"));
    EXPECT_TRUE(contains(*art, "feature"));
    EXPECT_TRUE(contains(*art, "┐"));
    EXPECT_TRUE(contains(*art, "┘") || contains(*art, "└"));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
}

TEST(MermaidTest, GitGraphMergeWithoutId) {
    const optional<mermaid_art> art =
            render_mermaid("gitGraph\n commit\n branch dev\n commit\n checkout main\n commit\n merge dev\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "merge dev"));
    EXPECT_TRUE(contains(*art, "dev"));
    EXPECT_GE(count_text(*art, "●"), size_t(3));
}

TEST(MermaidTest, PieChartBarsPercentagesAndTitle) {
    const optional<mermaid_art> art = render_mermaid("pie title Favorite Fruits\n"
                                                     " \"Apple\" : 42\n"
                                                     " \"Banana\" : 21\n"
                                                     " \"Cherry\" : 9\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    ASSERT_FALSE(art->lines.empty());
    EXPECT_TRUE(art->lines[0].view().starts_with("Favorite Fruits"));
    EXPECT_TRUE(role_text(*art, mermaid_role::title).view() == string_view("Favorite Fruits"));
    EXPECT_TRUE(contains(*art, "Apple"));
    EXPECT_TRUE(contains(*art, "Banana"));
    EXPECT_TRUE(contains(*art, "Cherry"));
    EXPECT_TRUE(contains(*art, "█"));
    EXPECT_TRUE(contains(*art, "42"));
    EXPECT_TRUE(contains(*art, "58%"));
    EXPECT_TRUE(contains(*art, "29%"));
    EXPECT_TRUE(contains(*art, "13%"));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
}

TEST(MermaidTest, PieChartRejectsUnquotedAndEmptyData) {
    EXPECT_FALSE(render_mermaid("pie\n A : 1\n").has_value());
    EXPECT_FALSE(render_mermaid("pie\n \"A\" : 0\n \"B\" : 0\n").has_value());
    EXPECT_FALSE(render_mermaid("pie\n").has_value());
}

TEST(MermaidTest, MindmapTreeConnectors) {
    const optional<mermaid_art> art = render_mermaid("mindmap\n"
                                                     "  root\n"
                                                     "    child one\n"
                                                     "      grandchild\n"
                                                     "    child two\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "root"));
    EXPECT_TRUE(contains(*art, "child one"));
    EXPECT_TRUE(contains(*art, "grandchild"));
    EXPECT_TRUE(contains(*art, "child two"));
    EXPECT_TRUE(contains(*art, "├─"));
    EXPECT_TRUE(contains(*art, "└─"));
    EXPECT_TRUE(contains(*art, "│  "));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
}

TEST(MermaidTest, MindmapShapesAreStripped) {
    const optional<mermaid_art> art = render_mermaid("mindmap\n"
                                                     "  root((Root Node))\n"
                                                     "    (Child)\n"
                                                     "    [Square]\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Root Node"));
    EXPECT_TRUE(contains(*art, "Child"));
    EXPECT_TRUE(contains(*art, "Square"));
    EXPECT_FALSE(contains(*art, "(("));
    EXPECT_FALSE(contains(*art, "["));
}

TEST(MermaidTest, TimelineSectionsAndContinuations) {
    const optional<mermaid_art> art = render_mermaid("timeline\n"
                                                     " title History\n"
                                                     " section 2000s\n"
                                                     "  2002 : LinkedIn\n"
                                                     "  2006 : Facebook\n"
                                                     "  2007 : Facebook : Google\n"
                                                     " section 2010s\n"
                                                     "  2010 : Instagram\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "History"));
    EXPECT_TRUE(contains(*art, "── 2000s ──"));
    EXPECT_TRUE(contains(*art, "── 2010s ──"));
    EXPECT_TRUE(contains(*art, "2002 ─ LinkedIn"));
    EXPECT_TRUE(contains(*art, "Facebook"));
    EXPECT_TRUE(contains(*art, "Google"));
    EXPECT_TRUE(contains(*art, "Instagram"));
    EXPECT_TRUE(has_role(*art, mermaid_role::title));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
}

TEST(MermaidTest, GanttSectionsBarsAndDates) {
    const optional<mermaid_art> art = render_mermaid("gantt\n"
                                                     " title Project\n"
                                                     " dateFormat YYYY-MM-DD\n"
                                                     " section Design\n"
                                                     " Task A :done, a1, 2024-01-01, 30d\n"
                                                     " Task B :a2, 2024-01-10, 2024-02-05\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    ASSERT_FALSE(art->lines.empty());
    EXPECT_TRUE(art->lines[0].view().starts_with("Project"));
    EXPECT_TRUE(contains(*art, "── Design ──"));
    EXPECT_TRUE(contains(*art, "Task A"));
    EXPECT_TRUE(contains(*art, "Task B"));
    EXPECT_TRUE(contains(*art, "█"));
    EXPECT_TRUE(contains(*art, "2024-01-01"));
    EXPECT_TRUE(contains(*art, "2024-01-31"));
    EXPECT_TRUE(contains(*art, "2024-02-05"));
    EXPECT_TRUE(contains(*art, "→"));
    EXPECT_TRUE(has_role(*art, mermaid_role::title));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge_label));
}

TEST(MermaidTest, GanttMilestoneAndInvalidTasks) {
    const optional<mermaid_art> milestone =
            render_mermaid("gantt\n section Plan\n Ship :milestone, m1, 2024-03-01, 0d\n");
    ASSERT_TRUE(milestone.has_value());
    expect_invariants(*milestone);
    EXPECT_TRUE(contains(*milestone, "◆"));
    EXPECT_TRUE(contains(*milestone, "2024-03-01"));
    EXPECT_TRUE(contains(*milestone, "Plan"));
    EXPECT_FALSE(render_mermaid("gantt\n title A\n").has_value());
    EXPECT_FALSE(render_mermaid("gantt\n section S\n Task :a1\n").has_value());
}

TEST(MermaidTest, JourneyScoresAndActors) {
    const optional<mermaid_art> art = render_mermaid("journey\n"
                                                     " title My Journey\n"
                                                     " section Work\n"
                                                     " Task A: 5: Me\n"
                                                     " Task B: 3: You\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "My Journey"));
    EXPECT_TRUE(contains(*art, "── Work ──"));
    EXPECT_TRUE(contains(*art, "Task A"));
    EXPECT_TRUE(contains(*art, "█████"));
    EXPECT_TRUE(contains(*art, "███░░"));
    EXPECT_TRUE(contains(*art, "5"));
    EXPECT_TRUE(contains(*art, "3"));
    EXPECT_TRUE(contains(*art, "(Me)"));
    EXPECT_TRUE(contains(*art, "(You)"));
}

TEST(MermaidTest, QuadrantChartGridAxesAndPoints) {
    const optional<mermaid_art> art = render_mermaid("quadrantChart\n"
                                                     " title Quadrants\n"
                                                     " x-axis Low --> High\n"
                                                     " y-axis Bottom --> Top\n"
                                                     " A: [0.3, 0.6]\n"
                                                     " B: [0.8, 0.2]\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "Quadrants"));
    EXPECT_TRUE(contains(*art, "x: Low → High"));
    EXPECT_TRUE(contains(*art, "y: Bottom → Top"));
    EXPECT_TRUE(contains(*art, "●"));
    EXPECT_TRUE(contains(*art, "A"));
    EXPECT_TRUE(contains(*art, "B"));
    EXPECT_TRUE(contains(*art, "┼"));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
    EXPECT_TRUE(has_role(*art, mermaid_role::edge));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
}

TEST(MermaidTest, RequirementDiagramKeyValueBox) {
    const optional<mermaid_art> art = render_mermaid("requirementDiagram\n"
                                                     " requirement r1 {\n"
                                                     "  id: 1\n"
                                                     "  text: The system shall work\n"
                                                     "  risk: high\n"
                                                     " }\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "r1"));
    EXPECT_TRUE(contains(*art, "id: 1"));
    EXPECT_TRUE(contains(*art, "text: The system shall work"));
    EXPECT_TRUE(contains(*art, "risk: high"));
    EXPECT_TRUE(contains(*art, "┌"));
    EXPECT_TRUE(contains(*art, "┘"));
    EXPECT_TRUE(has_role(*art, mermaid_role::border));
    EXPECT_TRUE(has_role(*art, mermaid_role::text));
}

TEST(MermaidTest, RequirementDiagramSingleLineBlockSplitsFields) {
    const optional<mermaid_art> art =
            render_mermaid("requirementDiagram\n"
                           " requirement r1 { id: 1 text: The system shall work risk: high }\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    EXPECT_TRUE(contains(*art, "id: 1"));
    EXPECT_TRUE(contains(*art, "text: The system shall work"));
    EXPECT_TRUE(contains(*art, "risk: high"));
}

TEST(MermaidTest, NewDiagramTypesWidthConsistency) {
    const char* sources[] = {
            "classDiagram\n class A\n class B\n A --> B : uses\n",
            "erDiagram\n C ||--o{ D : has\n",
            "gitGraph\n commit id: \"a\"\n branch b\n commit id: \"c\"\n checkout main\n merge b\n",
            "pie title Share\n \"A\" : 3\n \"B\" : 1\n",
            "mindmap\n  root\n    child\n",
            "timeline\n section S\n 2020 : event\n",
            "gantt\n section S\n Task :t1, 2024-01-01, 10d\n",
            "journey\n section S\n Task: 4: Me\n",
            "quadrantChart\n x-axis Low --> High\n A: [0.4, 0.7]\n",
            "requirementDiagram\n requirement r { id: 1 }\n",
    };
    for (const char* source: sources) {
        const optional<mermaid_art> art = render_mermaid(source);
        ASSERT_TRUE(art.has_value());
        int widest = 0;
        for (const string& line: art->lines) {
            widest = max(widest, text_display_width(line.view()));
        }
        EXPECT_EQ(art->width, widest);
        for (const string& line: art->lines) {
            EXPECT_EQ(text_display_width(line.view()), art->width);
        }
    }
}

TEST(MermaidTest, NewDiagramTypesStyledRunsReconstruct) {
    const char* sources[] = {
            "classDiagram\n class A { +int x }\n class B\n A --> B : uses\n",
            "erDiagram\n CUSTOMER ||--o{ ORDER : places\n",
            "gitGraph\n commit id: \"a\"\n branch b\n commit id: \"c\"\n merge b\n",
            "pie title Share\n \"A\" : 3\n \"B\" : 1\n",
            "mindmap\n  root\n    child\n",
            "timeline\n title T\n section S\n 2020 : event\n",
            "gantt\n title T\n section S\n Task :t1, 2024-01-01, 10d\n",
            "journey\n section S\n Task: 4: Me\n",
            "quadrantChart\n A: [0.4, 0.7]\n",
            "requirementDiagram\n requirement r {\n id: 1\n }\n",
    };
    for (const char* source: sources) {
        const optional<mermaid_art> art = render_mermaid(source);
        ASSERT_TRUE(art.has_value());
        EXPECT_EQ(art->lines.size(), art->styled.size());
        EXPECT_TRUE(runs_cover_lines(*art));
        EXPECT_TRUE(roles_are_valid(*art));
    }
}

TEST(MermaidTest, NewDiagramHeadersWithoutContentReturnNone) {
    const char* sources[] = {
            "classDiagram\n", "erDiagram\n", "gitGraph\n", "pie\n",           "mindmap\n",
            "timeline\n",     "gantt\n",     "journey\n",  "quadrantChart\n", "requirementDiagram\n",
    };
    for (const char* source: sources) {
        EXPECT_FALSE(render_mermaid(source).has_value());
    }
}

TEST(MermaidTest, UnsupportedBetaDiagramTypesReturnNone) {
    EXPECT_FALSE(render_mermaid("sankey-beta\n A,B,10\n").has_value());
    EXPECT_FALSE(render_mermaid("xychart-beta\n x-axis [1, 2]\n").has_value());
    EXPECT_FALSE(render_mermaid("block-beta\n a\n").has_value());
    EXPECT_FALSE(render_mermaid("C4Context\n Person(a, \"A\")\n").has_value());
}

TEST(MermaidTest, NewDiagramTypesHonourFrontmatterTitle) {
    const optional<mermaid_art> art = render_mermaid("---\ntitle: Pie Overview\n---\npie\n \"A\" : 3\n \"B\" : 1\n");
    ASSERT_TRUE(art.has_value());
    expect_invariants(*art);
    ASSERT_FALSE(art->lines.empty());
    EXPECT_TRUE(art->lines[0].view().starts_with("Pie Overview"));
    EXPECT_TRUE(role_text(*art, mermaid_role::title).view() == string_view("Pie Overview"));
}

TEST(MermaidTest, NewDiagramTypesAreDeterministic) {
    const char* sources[] = {
            "classDiagram\n class A { +int x }\n class B\n A --> B : uses\n",
            "erDiagram\n CUSTOMER ||--o{ ORDER : places\n",
            "gitGraph\n commit id: \"a\"\n branch b\n commit id: \"c\"\n merge b\n",
            "pie title Share\n \"A\" : 3\n \"B\" : 1\n",
            "mindmap\n  root\n    child\n",
            "timeline\n section S\n 2020 : event\n",
            "gantt\n section S\n Task :t1, 2024-01-01, 10d\n",
            "journey\n section S\n Task: 4: Me\n",
            "quadrantChart\n A: [0.4, 0.7]\n",
            "requirementDiagram\n requirement r {\n id: 1\n }\n",
    };
    for (const char* source: sources) {
        const optional<mermaid_art> first = render_mermaid(source);
        const optional<mermaid_art> second = render_mermaid(source);
        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(second.has_value());
        ASSERT_EQ(first->lines.size(), second->lines.size());
        ASSERT_EQ(first->styled.size(), second->styled.size());
        for (size_t i = 0; i < first->lines.size(); ++i) {
            EXPECT_EQ(first->lines[i], second->lines[i]);
            ASSERT_EQ(first->styled[i].size(), second->styled[i].size());
            for (size_t j = 0; j < first->styled[i].size(); ++j) {
                EXPECT_EQ(first->styled[i][j].begin, second->styled[i][j].begin);
                EXPECT_EQ(first->styled[i][j].end, second->styled[i][j].end);
                EXPECT_EQ(first->styled[i][j].role, second->styled[i][j].role);
            }
        }
    }
}
