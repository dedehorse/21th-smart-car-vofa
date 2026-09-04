/**
  ******************************************************************************
  * @file    LQ_PathPlan.c
  * @brief   节点法路径规划模块实现
  *          状态机: PP_NONE → (确认T字路口+查表) → 计数+冷却 → PP_NONE
  ******************************************************************************
  */

#include "LQ_PathPlan.h"
#include "lq_include.h"

/* ══════════════════════════════════════════════════════════════════════════
 * 参数 (可在 config.h 中覆盖)
 * ══════════════════════════════════════════════════════════════════════════ */
#ifndef PP_EXIT_FRAMES
#define PP_EXIT_FRAMES       8     /* 退出确认帧数: 连续N帧无路口→确认已通过  */
#endif
#ifndef PP_LOCK_TIMEOUT
#define PP_LOCK_TIMEOUT     100    /* 元素锁超时帧数: 超过此值→跳过该节点     */
#endif
#ifndef PP_SAME_COOLDOWN
#define PP_SAME_COOLDOWN      5    /* 同类型路口触发后的冷却帧数                 */
#endif
#ifndef PP_STRAIGHT_DISTANCE
#define PP_STRAIGHT_DISTANCE 20.0f /* 直行通过路口后的解锁距离 (cm)              */
#endif
#ifndef PP_CONFIRM_FRAMES
#define PP_CONFIRM_FRAMES      2
#endif
#ifndef PP_NODE3_CONFIRM_FRAMES
#define PP_NODE3_CONFIRM_FRAMES 5
#endif
#ifndef PP_SYMMETRICAL_T_CONFIRM_FRAMES
#define PP_SYMMETRICAL_T_CONFIRM_FRAMES 10
#endif
#ifndef PP_NODE7_CONFIRM_FRAMES
#define PP_NODE7_CONFIRM_FRAMES 3
#endif
#ifndef PP_TURN_ANGLE_T
#define PP_TURN_ANGLE_T        65
#endif
#ifndef PP_TURN_ANGLE_TRIODE_T
#define PP_TURN_ANGLE_TRIODE_T 75
#endif
#ifndef PP_TURNING_TIMEOUT
#define PP_TURNING_TIMEOUT  300    /* 转弯超时帧数: 超过此值→强制退出          */
#endif
#ifndef PP_COOLDOWN
#define PP_COOLDOWN          10    /* 退出后冷却帧数: 防止紧邻路口重复计数     */
#endif

#define ARRAY_LEN(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

#define PP_STARTUP_FRAMES  60    /* 启动后忽略前N帧, 等摄像头稳定 */

/* ══════════════════════════════════════════════════════════════════════════
 * 全局变量 — path_decision() 每帧读取
 * ══════════════════════════════════════════════════════════════════════════ */
uint8_t g_pp_action    = ACT_NONE;
uint8_t g_pp_lock_type = 0;        /* 0=不锁, 1-9=只接受该类型 */
float   g_straight_yaw_ref = 0;    /* 直行通过路口时的参考yaw角 */
float   g_straight_lock_mileage = 0.0f; /* 直行触发时的里程, 用于距离解锁 */

/* ══════════════════════════════════════════════════════════════════════════
 * 内部状态
 * ══════════════════════════════════════════════════════════════════════════ */
static uint8_t         pp_node_count  = 0;       /* 已通过的 T 字路口数        */
static uint16_t        pp_frame_cnt   = 0;       /* 锁超时已持续帧数            */
static uint8_t         pp_cooldown_cnt = 0;      /* 冷却计数                  */
static uint8_t         pp_last_type   = 0;       /* 上一个路口的类型, 用于冷却期智能重置 */
static uint16_t        pp_startup_cnt = 0;       /* 启动稳定帧计数              */
static uint8_t         pp_just_triggered = 0;    /* 刚触发过节点, 需等距离/转角达标才解锁 */
static uint8_t         pp_same_cooldown  = 0;    /* 同类型冷却计数                  */
static uint8_t         pp_triggered_type = 0;    /* 最近触发的路口类型, 用于冷却判断  */
static uint8_t         pp_confirm_type   = 0;    /* 正在连续确认的基础节点类型        */
static uint8_t         pp_confirm_count  = 0;    /* 目标节点已连续出现的帧数           */
static float           pp_turn_angle     = PP_TURN_ANGLE_T; /* 本次规划转弯完成角 */
/* ══════════════════════════════════════════════════════════════════════════
 * 路由表 — 根据实际赛道配置 (expected_type, action)
 *
 * expected_type: 期望路口类型 (1=L90,2=R90,3=T字,4=左T,5=右T,6=十字,
 *                7=原三极管,8=左三极管T,9=右三极管T,0=任意)
 * action:        ACT_TURN_LEFT(左转), ACT_TURN_RIGHT(右转),
 *                ACT_GO_STRAIGHT(直行忽略路口)
 *
 * 【使用方法】:
 *   跑一圈记录每个路口类型 → 决定每个路口怎么走 → 填入下表
 *
 *   node_count 从0开始, 检测到第1个路口→node_count=1→查 route[node_count%length]
 *   多圈自动循环(取模)
 * ══════════════════════════════════════════════════════════════════════════ */

/* ══════════════════════════════════════════════════════════════════════════
 * 路由表: 按实际赛道配置, 每行 = {期望路口类型, 规划动作}
 *
 * 路口类型:  1=L90, 2=R90, 3=对称T, 4=左T, 5=右T, 6=十字,
 *            7=原三极管, 8=左三极管T, 9=右三极管T, 0=任意
 * 规划动作:  ACT_TURN_LEFT(左拉线), ACT_TURN_RIGHT(右拉线), ACT_GO_STRAIGHT(直行)
 *
 * node_count 从0开始, 检测到第1个路口→node_count=1→查 route[node_count%length]
 * 多圈自动循环(取模)
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── 0: 全右T直行路线 (11个右T, 全部直行通过) ── */
static const RouteNode_t route_414_nodes[] = {
    /* {期望类型, 动作}                        说明                     */
    {5, ACT_GO_STRAIGHT},    /*  1: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  2: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  3: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  4: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  5: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  6: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  7: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  8: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /*  9: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /* 10: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /* 11: 右T       → 直行                 */
};

/* ── 1: 原始路线 (23节点, 跑完自动停车) ── */
static const RouteNode_t route_8_nodes[] = {
    {5, ACT_TURN_RIGHT},     /*  1: 右T       → 右转                 */
    {4, ACT_TURN_LEFT},      /*  2: 左T       → 左转                 */
    {4, ACT_GO_STRAIGHT},    /*  3: 左T       → 直行                 */
    {3, ACT_TURN_LEFT},      /*  4: 对称T     → 左转                 */
    {4, ACT_GO_STRAIGHT},    /*  5: 左T       → 直行                 */
    {4, ACT_GO_STRAIGHT},    /*  6: 左T       → 直行                 */
    {4, ACT_TURN_LEFT},      /*  7: 左T       → 左转                 */
    {4, ACT_TURN_LEFT},      /*  8: 左T       → 左转                 */
    {5, ACT_TURN_RIGHT},     /*  9: 右T       → 右转                 */
    {3, ACT_TURN_LEFT},      /* 10: 对称T     → 左转                 */
    {4, ACT_TURN_LEFT},      /* 11: 左T       → 左转                 */
    {4, ACT_GO_STRAIGHT},    /* 12: 左T       → 直行                 */
    {3, ACT_TURN_LEFT},      /* 13: 对称T     → 左转                 */
    {3, ACT_TURN_RIGHT},     /* 14: 对称T     → 右转                 */
    {3, ACT_TURN_LEFT},      /* 15: 对称T     → 左转                 */
    {3, ACT_TURN_LEFT},      /* 16: 对称T     → 左转                 */
    {4, ACT_GO_STRAIGHT},    /* 17: 左T       → 直行                 */
    {4, ACT_GO_STRAIGHT},    /* 18: 左T       → 直行                 */
    {4, ACT_TURN_LEFT},      /* 19: 左T       → 左转                 */
    {4, ACT_GO_STRAIGHT},    /* 20: 左T       → 直行                 */
    {3, ACT_TURN_RIGHT},     /* 21: 对称T     → 右转                 */
    {5, ACT_GO_STRAIGHT},    /* 22: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /* 23: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /* 24: 右T       → 直行                 */
};

/* ── 2: 无锁路线 (22节点T字路口) ── */
static const RouteNode_t route_nolock_nodes[] = {
    /* {期望类型, 动作}                        说明                     */
    {5, ACT_TURN_RIGHT},     /*  1: 右T       → 右转                 */
    {4, ACT_TURN_LEFT},      /*  2: 左T       → 左转                 */
    {4, ACT_TURN_LEFT},      /*  3: 左T       → 左转                 */
    {5, ACT_TURN_RIGHT},     /*  4: 右T       → 右转                 */
    {5, ACT_GO_STRAIGHT},    /*  5: 右T       → 直行                 */
    {3, ACT_TURN_LEFT},      /*  6: 对称T     → 左转                 */
    {4, ACT_TURN_LEFT},      /*  7: 左T       → 左转                 */
    {4, ACT_TURN_LEFT},      /*  8: 左T       → 左转                 */
    {5, ACT_TURN_RIGHT},     /*  9: 右T       → 右转                 */
    {3, ACT_TURN_RIGHT},     /* 10: 对称T     → 右转                 */
    {5, ACT_TURN_RIGHT},     /* 11: 右T       → 右转                 */
    {5, ACT_GO_STRAIGHT},    /* 12: 右T       → 直行                 */
    {3, ACT_TURN_LEFT},      /* 13: 对称T     → 左转                 */
    {3, ACT_TURN_LEFT},      /* 14: 对称T     → 左转                 */
    {4, ACT_GO_STRAIGHT},    /* 15: 左T       → 直行                 */
    {4, ACT_GO_STRAIGHT},    /* 16: 左T       → 直行                 */
    {4, ACT_TURN_LEFT},      /* 17: 左T       → 左转                 */
    {4, ACT_GO_STRAIGHT},    /* 18: 左T       → 直行                 */
    {3, ACT_TURN_RIGHT},     /* 19: 对称T     → 右转                 */
    {5, ACT_GO_STRAIGHT},    /* 20: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /* 21: 右T       → 直行                 */
    {5, ACT_GO_STRAIGHT},    /* 22: 右T       → 直行                 */
};

#define MAX_ROUTE_COUNT 4

static const PathPlan_t g_routes[MAX_ROUTE_COUNT] = {
    { route_414_nodes,   ARRAY_LEN(route_414_nodes)   },  /* 0 = 414路线(备用)  */
    { route_8_nodes,     ARRAY_LEN(route_8_nodes)     },  /* 1 = 原始23节点路线  */
    { route_nolock_nodes, ARRAY_LEN(route_nolock_nodes) }, /* 2 = 无锁路线       */
    { NULL, 0 },                                          /* 3 = 预留           */
};

/* 默认使用原始路线 */
static const PathPlan_t *current_plan = &g_routes[1];

/* ══════════════════════════════════════════════════════════════════════════
 * API 实现
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  初始化路径规划模块
 */
void PathPlan_Init(void)
{
    pp_node_count   = 0;
    pp_frame_cnt    = 0;
    pp_cooldown_cnt = 0;
    pp_last_type    = 0;
    pp_startup_cnt  = 0;
    pp_just_triggered = 0;
    pp_same_cooldown  = 0;
    pp_triggered_type = 0;
    pp_confirm_type   = 0;
    pp_confirm_count  = 0;
    pp_turn_angle     = PP_TURN_ANGLE_T;

    g_pp_action    = ACT_NONE;
    g_pp_lock_type = 0;
    g_straight_lock_mileage = 0.0f;

    extern uint8_t g_stop_active;
    g_stop_active = 0;
}

/**
 * @brief  切换到指定路线
 * @param  index: 路线索引
 */
void PathPlan_SetRoute(uint8_t index)
{
    if (index < MAX_ROUTE_COUNT &&
        g_routes[index].nodes != NULL &&
        g_routes[index].length > 0)
    {
        current_plan = &g_routes[index];
        PathPlan_Init();  /* 重置所有状态 */
    }
}

/**
 * @brief  获取当前已通过路口数 (即 node_count)
 */
uint8_t PathPlan_GetNode(void)
{
    return pp_node_count;
}

uint8_t PathPlan_GetState(void)
{
    return 0;
}

const char* PathPlan_GetStateName(void)
{
    return "LOCK";
}

/**
 * @brief  获取路线数量
 */
uint8_t PathPlan_GetRouteCount(void)
{
    return MAX_ROUTE_COUNT;
}

uint8_t PathPlan_GetRouteLength(void)
{
    return (current_plan) ? current_plan->length : 0;
}

uint8_t PathPlan_GetRouteEntry(uint8_t index, uint8_t *expected_type, uint8_t *action)
{
    if (!current_plan || index >= current_plan->length) return 0;
    *expected_type = current_plan->nodes[index].expected_type;
    *action        = current_plan->nodes[index].action;
    return 1;
}

/* 除0表示任意类型外, 普通T与左右三极管T均严格按类型匹配。 */
uint8_t PathPlan_TypeMatches(uint8_t expected_type, uint8_t detected_type)
{
    if (expected_type == 0 || expected_type == detected_type) return 1;
    return 0;
}

float PathPlan_GetTurnAngle(void)
{
    return pp_turn_angle;
}

uint8_t PathPlan_GetTriggeredType(void)
{
    return pp_triggered_type;
}

/**
 * @brief  每帧调用, 推进路径规划状态机
 * @param  stable_turn: check_turn() 当前帧识别的路口类型 (0-9)
 * @param  turn_completed: path_decision() 的陀螺仪转角完成标志
 *
 * @note   必须在 path_decision() 之后, s_turn_flag 计算之后调用
 *         本函数更新 g_pp_action / g_pp_lock_type, 供下一帧 path_decision() 读取
 */
void PathPlan_Process(uint8_t stable_turn, uint8_t turn_completed)
{
    /* 已停车 → 不再处理任何路口 */
    if (g_pp_action == ACT_STOP) {
        return;
    }

    pp_frame_cnt++;

    /* ── 启动保护: 上电后前N帧忽略所有路口 ── */
    if (pp_startup_cnt < PP_STARTUP_FRAMES) {
        pp_startup_cnt++;
        g_pp_lock_type = 0;
        pp_frame_cnt = 0;
        return;
    }

    /* ── 冷却计数每帧递减 ── */
    if (pp_same_cooldown > 0) pp_same_cooldown--;

    /* ── 触发后锁定: 转弯看陀螺仪完成标志, 直行看行驶距离 ── */
    if (pp_just_triggered) {
        uint8_t unlock = 0;
        if (g_pp_action == ACT_GO_STRAIGHT) {
            /* 直行: 行驶够距离解锁 */
            if (Get_Distance() - g_straight_lock_mileage >= PP_STRAIGHT_DISTANCE) {
                unlock = 1;
            }
        } else {
            /* 转弯: 仅在 path_decision 确认达到指定角度后解锁 */
            if (turn_completed) unlock = 1;
        }
        if (unlock) {
            pp_just_triggered = 0;
            g_pp_action       = ACT_NONE;
            pp_frame_cnt      = 0;

            /* 解锁后路口仍在视野且下一节点同类型 → 跳过冷却 */
            if (stable_turn != 0) {
                uint8_t ni = pp_node_count % current_plan->length;
                if (PathPlan_TypeMatches(current_plan->nodes[ni].expected_type,
                                         pp_triggered_type)) {
                    pp_same_cooldown = 0;
                } else {
                    pp_same_cooldown = PP_SAME_COOLDOWN;
                }
            } else {
                pp_same_cooldown = PP_SAME_COOLDOWN;
            }

            /* 转弯完成后陀螺仪清零, 防止累积漂移 */
            Reset_Current_Yaw();

            /* 全部节点走完 → 自动停车 */
            if (pp_node_count >= current_plan->length) {
                g_pp_action    = ACT_STOP;
                g_pp_lock_type = 0;
                return;
            }
        } else {
            return;
        }
    }

    /* ── 预设元素锁: 永远锁定当前期望的 T 字路口类型 ── */
    {
        uint8_t idx = pp_node_count % current_plan->length;
        g_pp_lock_type = current_plan->nodes[idx].expected_type;
    }

    /* 没有路口 → 重置 */
    if (stable_turn == 0) {
        pp_frame_cnt = 0;
        pp_confirm_type  = 0;
        pp_confirm_count = 0;
        return;
    }

    /* 类型1/2 (左转/右转) → 不算节点, 临时解锁让 path_decision 自然处理 */
    if (stable_turn <= 2) {
        g_pp_lock_type = 0;
        pp_frame_cnt = 0;
        pp_confirm_type  = 0;
        pp_confirm_count = 0;
        return;
    }

    /* ── 类型3-9 → 连续确认后再查路由表并触发动作 ── */
    {
        uint8_t idx = pp_node_count % current_plan->length;
        const RouteNode_t *node = &current_plan->nodes[idx];
        uint8_t confirm_frames = PP_CONFIRM_FRAMES;

        /* 对称T字路口统一使用独立确认帧数。 */
        if (node->expected_type == 3) {
            confirm_frames = PP_SYMMETRICAL_T_CONFIRM_FRAMES;
        }

        /* 原始路线个别节点使用独立确认帧数。 */
        if (current_plan == &g_routes[1]) {
            if (idx == 2) {
                confirm_frames = PP_NODE3_CONFIRM_FRAMES;
            } else if (idx == 6) {
                confirm_frames = PP_NODE7_CONFIRM_FRAMES;
            }
        }

        /* 同类型冷却: 冷却期内同类型路口直接忽略 */
        if (pp_same_cooldown > 0
            && stable_turn == pp_triggered_type) {
            pp_confirm_type  = 0;
            pp_confirm_count = 0;
            return;
        }

        /* 元素锁: 不匹配 → 等待或超时跳过 */
        if (!PathPlan_TypeMatches(node->expected_type, stable_turn)) {
            pp_confirm_type  = 0;
            pp_confirm_count = 0;
            if (pp_frame_cnt > PP_LOCK_TIMEOUT) {
                pp_node_count++;
                pp_frame_cnt = 0;
            }
            return;        }

        /* 只有完全相同的目标节点类型连续出现N帧, 才触发计数和规划动作。 */
        {
            if (stable_turn == pp_confirm_type) {
                if (pp_confirm_count < confirm_frames) pp_confirm_count++;
            } else {
                pp_confirm_type  = stable_turn;
                pp_confirm_count = 1;
            }
            if (pp_confirm_count < confirm_frames) return;
        }

        /* ── 匹配! 计数器+1, 设动作, 锁自动切换到下一个期望类型 ── */
        pp_node_count++;
        pp_just_triggered  = 1;       /* 上锁, 等距离/转角达标才解锁 */
        pp_triggered_type  = stable_turn;
        pp_confirm_type    = 0;
        pp_confirm_count   = 0;

        /* 显式左右三极管T使用更大的完成角。 */
        if (stable_turn == 8 || stable_turn == 9) {
            pp_turn_angle = PP_TURN_ANGLE_TRIODE_T;
        } else {
            pp_turn_angle = PP_TURN_ANGLE_T;
        }

        g_pp_action    = node->action;
        g_pp_lock_type = node->expected_type;

        /* 记录直行参考角度, 用于确保直行通过路口时角度不变 */
        if (node->action == ACT_GO_STRAIGHT) {
            g_straight_yaw_ref      = Get_Current_Yaw();
            g_straight_lock_mileage = Get_Distance();
        }

        pp_frame_cnt = 0;
    }
}
