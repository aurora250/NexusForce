set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

DIRECTORIES=("include" "src" "tests")
EXTENSIONS=("*.c" "*.cpp" "*.h" "*.hpp" "*.py" "*.java" "*.js" "*.ts" "*.sh")

has_cloc=false
if command -v cloc &>/dev/null; then
    has_cloc=true
fi

check_directories() {
    local missing_dirs=()
    for dir in "${DIRECTORIES[@]}"; do
        if [[ ! -d "$dir" ]]; then
            missing_dirs+=("$dir")
        fi
    done

    if [[ ${#missing_dirs[@]} -gt 0 ]]; then
        echo -e "${YELLOW}警告: 以下目录不存在: ${missing_dirs[*]}${NC}"
        return 1
    fi
    return 0
}

count_with_cloc() {
    echo -e "${CYAN}使用 cloc 统计...${NC}"
    echo "================================"

    local existing_dirs=()
    for dir in "${DIRECTORIES[@]}"; do
        [[ -d "$dir" ]] && existing_dirs+=("$dir")
    done

    if [[ ${#existing_dirs[@]} -eq 0 ]]; then
        echo -e "${RED}错误: 没有找到任何目录${NC}"
        return 1
    fi

    local ext_list=""
    for ext in "${EXTENSIONS[@]}"; do
        ext="${ext#\*\.}"
        if [[ -n "$ext_list" ]]; then
            ext_list+=","
        fi
        ext_list+="$ext"
    done

    cloc "${existing_dirs[@]}" --include-ext="$ext_list"
}

count_manually() {
    echo -e "${CYAN}手动统计代码行数...${NC}"
    echo "================================"

    local total_files=0
    local total_lines=0
    local total_code_lines=0
    local total_comment_lines=0
    local total_blank_lines=0

    local ext_conditions=""
    for ext in "${EXTENSIONS[@]}"; do
        ext="${ext#\*}"
        [[ -n "$ext_conditions" ]] && ext_conditions+=" -o"
        ext_conditions+=" -name \"*${ext}\""
    done

    for dir in "${DIRECTORIES[@]}"; do
        if [[ ! -d "$dir" ]]; then
            continue
        fi

        echo -e "\n${YELLOW}--- $dir ---${NC}"

        local files
        files=$(eval "find \"$dir\" -type f \( $ext_conditions \)" 2>/dev/null)
        if [[ -z "$files" ]]; then
            echo "  没有找到匹配的文件"
            continue
        fi

        local dir_files=0
        local dir_lines=0

        while IFS= read -r file; do
            [[ -z "$file" ]] && continue

            dir_files=$((dir_files + 1))

            local lines
            lines=$(wc -l < "$file" 2>/dev/null || echo 0)
            dir_lines=$((dir_lines + lines))

            local comment_lines=0
            local blank_lines=0

            local file_ext="${file##*.}"

            case "$file_ext" in
                c|cpp|h|hpp|java|js|ts|css|scss|php|go|rs|swift|kt|scala)
                    comment_lines=$(grep -cE '^\s*//|^\s*/\*|^\s*\*|^\s*\*/' "$file" 2>/dev/null || echo 0)
                    ;;
                py|rb|pl|sh|bash|zsh|yaml|yml|toml)
                    comment_lines=$(grep -cE '^\s*#' "$file" 2>/dev/null || echo 0)
                    ;;
                sql|lua|haskell)
                    comment_lines=$(grep -cE '^\s*--' "$file" 2>/dev/null || echo 0)
                    ;;
                *)
                    comment_lines=0
                    ;;
            esac

            blank_lines=$(grep -cE '^\s*$' "$file" 2>/dev/null || echo 0)

            total_comment_lines=$((total_comment_lines + comment_lines))
            total_blank_lines=$((total_blank_lines + blank_lines))

        done <<< "$files"

        echo "  文件数: $dir_files"
        echo "  总行数: $dir_lines"

        total_files=$((total_files + dir_files))
        total_lines=$((total_lines + dir_lines))
    done

    total_code_lines=$((total_lines - total_comment_lines - total_blank_lines))

    echo -e "\n${GREEN}================================"
    echo "统计汇总"
    echo "================================"
    echo -e "总文件数:     $total_files"
    echo -e "总行数:       $total_lines"
    echo -e "代码行数:     $total_code_lines"
    echo -e "注释行数:     $total_comment_lines"
    echo -e "空行数:       $total_blank_lines${NC}"
}

count_simple() {
    echo -e "${CYAN}快速统计代码行数...${NC}"
    echo "================================"

    local total_files=0
    local total_lines=0

    local ext_conditions=""
    for ext in "${EXTENSIONS[@]}"; do
        ext="${ext#\*}"
        [[ -n "$ext_conditions" ]] && ext_conditions+=" -o"
        ext_conditions+=" -name \"*${ext}\""
    done

    for dir in "${DIRECTORIES[@]}"; do
        if [[ ! -d "$dir" ]]; then
            echo -e "${YELLOW}$dir: 目录不存在${NC}"
            continue
        fi

        local dir_files
        dir_files=$(eval "find \"$dir\" -type f \( $ext_conditions \)" 2>/dev/null | wc -l)
        local dir_lines
        dir_lines=$(eval "find \"$dir\" -type f \( $ext_conditions \) -exec cat {} +" 2>/dev/null | wc -l)

        echo -e "  $dir: ${GREEN}$dir_lines${NC} 行 ($dir_files 个文件)"

        total_files=$((total_files + dir_files))
        total_lines=$((total_lines + dir_lines))
    done

    echo "--------------------------------"
    echo -e "总计: ${GREEN}$total_lines${NC} 行 ($total_files 个文件)"
}

show_help() {
    cat << EOF
用法: $0 [选项]

统计 include、src、tests 目录中的代码行数。

选项:
    -h, --help     显示此帮助信息
    -s, --simple   快速统计模式（只统计总行数和文件数）
    -m, --manual   手动统计模式（区分代码、注释和空行）
    -c, --cloc     使用 cloc 工具统计（如果可用）
    -e, --ext      指定文件扩展名（逗号分隔，如: cpp,h,py）

默认行为：
    - 如果安装了 cloc，优先使用 cloc 进行详细统计
    - 否则使用简单统计模式

示例:
    $0                  # 默认统计
    $0 -s               # 快速统计
    $0 -e "cpp,hpp,h"  # 只统计 C++ 文件
    $0 -m               # 手动统计并区分代码类型
EOF
}

main() {
    local mode="auto"

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -s|--simple)
                mode="simple"
                shift
                ;;
            -m|--manual)
                mode="manual"
                shift
                ;;
            -c|--cloc)
                mode="cloc"
                shift
                ;;
            -e|--ext)
                IFS=',' read -ra EXTENSIONS <<< "$2"
                for i in "${!EXTENSIONS[@]}"; do
                    EXTENSIONS[$i]="*.${EXTENSIONS[$i]}"
                done
                shift 2
                ;;
            *)
                echo -e "${RED}未知选项: $1${NC}"
                show_help
                exit 1
                ;;
        esac
    done

    if ! check_directories; then
        echo -e "${YELLOW}继续统计已存在的目录...${NC}\n"
    fi

    case $mode in
        auto)
            if $has_cloc; then
                count_with_cloc
            else
                echo -e "${YELLOW}提示: 安装 cloc 可获得更详细的统计信息${NC}"
                echo -e "${YELLOW}安装方法: sudo apt install cloc (Ubuntu/Debian) 或 brew install cloc (macOS)${NC}\n"
                count_simple
            fi
            ;;
        simple)
            count_simple
            ;;
        manual)
            count_manually
            ;;
        cloc)
            if $has_cloc; then
                count_with_cloc
            else
                echo -e "${RED}错误: cloc 未安装${NC}"
                echo -e "安装方法: sudo apt install cloc (Ubuntu/Debian) 或 brew install cloc (macOS)"
                exit 1
            fi
            ;;
    esac
}

main "$@"
