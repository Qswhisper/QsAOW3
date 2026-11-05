"""Read-only validation/JSON export of v1..v16 growth and reward journals."""
import argparse
import json
import struct
from pathlib import Path


def decode(path):
    data = path.read_bytes()
    if len(data) not in (8256, 41024, 45128,47184,51288,52336,52344,54064,54584,54592,56328,58072,60280):
        raise ValueError(f'{path}: unsupported length {len(data)}')
    magic, version, size, stored = struct.unpack_from('<4I', data)
    if magic != 0x33545241 or (version, size) not in ((1, 8256), (2, 41024), (3, 45128), (4, 45128), (5, 45128),(6,47184),(7,51288),(8,52336),(9,52344),(10,54064),(11,54064),(12,54584),(13,54592),(14,56328),(15,58072),(16,60280)) or size != len(data):
        raise ValueError(f'{path}: unsupported header/version')
    checksum_data = bytearray(data)
    checksum_data[12:16] = bytes(4)
    checksum = 2166136261
    for byte in checksum_data:
        checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
    if checksum != stored:
        raise ValueError(f'{path}: checksum mismatch')
    generation, next_run, settled_run, credits, gold, xp, stars, count, upgrade_count = struct.unpack_from('<3Q4i2I', data, 16)
    if count > 256:
        raise ValueError(f'{path}: invalid mission count')
    missions = []
    for index in range(count):
        mid, wins, losses, best_stars, best_xp, earned, objective_count, objectives = struct.unpack_from('<6i2I', data, 64 + index * 32)
        if objective_count > 32:
            raise ValueError(f'{path}: invalid objective count')
        missions.append(dict(id=mid, wins=wins, losses=losses, stars=best_stars,
                             xp=best_xp, earned_credits=earned,
                             objectives=[bool(objectives & (1 << i)) for i in range(objective_count)]))
    if upgrade_count > (8192 if version >= 2 else 0):
        raise ValueError(f'{path}: invalid upgrade count')
    upgrades = list(struct.unpack_from(f'<{upgrade_count}i', data, 8256)) if upgrade_count else []
    receipts, stocks = [], []
    if version >= 3:
        receipt_count, stock_count = struct.unpack_from('<2I', data, 41024)
        if receipt_count > 256 or stock_count > 256:
            raise ValueError(f'{path}: invalid career counts')
        receipts = list(struct.unpack_from(f'<{receipt_count}i', data, 41032))
        for i in range(stock_count):
            kind, key, amount = struct.unpack_from('<3i', data, 42056 + 12 * i)
            if kind not in ((4,7,8,9,10,11,17) if version>=12 else (4,7,8,9,10,11) if version>=11 else (4, 8, 9) if version >= 5 else (4, 9) if version >= 4 else (4,)) or key < 0 or amount <= 0:
                raise ValueError(f'{path}: invalid inventory item')
            stocks.append(dict(type=kind, id=key, amount=amount))
        if any(x < 0 for x in receipts) or len(set(receipts)) != len(receipts) or len({(x['type'], x['id']) for x in stocks}) != len(stocks):
            raise ValueError(f'{path}: duplicate career receipt or inventory key')
    skill_nodes=[]
    if version>=6:
        n=struct.unpack_from('<I',data,45128)[0]
        if n>512:raise ValueError(f'{path}: invalid skill count')
        skill_nodes=list(struct.unpack_from(f'<{n}i',data,45132))
        if any(x<=0 for x in skill_nodes) or len(set(skill_nodes))!=n:raise ValueError(f'{path}: invalid skill receipts')
    quest_ids=[];quest_warpoints=0
    if version>=7:
        n=struct.unpack_from('<I',data,47184)[0]
        if n>512:raise ValueError(f'{path}: invalid quest count')
        quest_ids=[dict(zip(('id','state'),struct.unpack_from('<2i',data,47188+i*8))) for i in range(n)]
        quest_warpoints=struct.unpack_from('<i',data,51284)[0]
        if any(x['id']<0 or x['state'] not in (8,32) for x in quest_ids) or len({x['id'] for x in quest_ids})!=n or quest_warpoints<0:raise ValueError(f'{path}: invalid quest receipts')
    contracts=[];contract_points=0;contract_pvp_cursor=0;contract_pvp_wins=0
    if version>=8:
        n,contract_points,contract_pvp_cursor,contract_pvp_wins=struct.unpack_from('<IiQQ',data,51288)
        if n>64 or contract_points<0:raise ValueError(f'{path}: invalid contract header')
        contracts=[dict(zip(('id','claims','baseline'),struct.unpack_from('<iIQ',data,51312+i*16))) for i in range(n)]
        if any(r['id']<0 for r in contracts) or len({r['id'] for r in contracts})!=n:raise ValueError(f'{path}: invalid contract receipts')
    warpoint_pvp_cursor=struct.unpack_from('<Q',data,52336)[0] if version>=9 else 0
    supply=dict(cycle=0,claims=0,group=0,step=0,tokens=0,last_box=0,last_cost=0,pending=False,last_cycle=0,draws=[],grants=[])
    if version>=10:
        fields=struct.unpack_from('<QQ5i3IQ',data,52344)
        cycle,claims,group,step,tokens,last_box,last_cost,draw_count,grant_count,pending,last_cycle=fields
        if tokens<0 or group<0 or not 0<=step<=12 or (group==0)!=(step==0) or draw_count>16 or grant_count>128 or pending not in (0,1) or last_cycle>cycle or cycle-last_cycle>1:raise ValueError('invalid supply journal header')
        if not claims and any([cycle,group,step,last_box,last_cost,draw_count,grant_count,pending,last_cycle]):raise ValueError('unclaimed supply has receipt data')
        if claims and (last_box<=0 or last_cost<=0 or not draw_count or not grant_count or cycle>claims):raise ValueError('invalid supply receipt')
        draws=[dict(zip(('item','amount'),struct.unpack_from('<2i',data,52400+i*8))) for i in range(draw_count)]
        grants=[dict(zip(('type','id','amount'),struct.unpack_from('<3i',data,52528+i*12))) for i in range(grant_count)]
        if any(r['item']<=0 or r['amount']<=0 for r in draws) or len({r['item'] for r in draws})!=draw_count:raise ValueError('invalid supply draw')
        if any(r['type'] not in ((0,1,2,4,7,8,9,10,11,17) if version>=12 else (0,1,2,4,7,8,9,10,11) if version>=11 else (0,1,2,4,8,9)) or r['id']<0 or r['amount']<=0 or (r['type']<=2 and r['id']) for r in grants):raise ValueError('invalid supply grant')
        supply=dict(cycle=cycle,claims=claims,group=group,step=step,tokens=tokens,last_box=last_box,last_cost=last_cost,pending=bool(pending),last_cycle=last_cycle,draws=draws,grants=grants)
    equipment=[]
    if version>=12:
        n,reserved=struct.unpack_from('<2I',data,54064)
        if n>64 or reserved:raise ValueError('invalid modification equipment header')
        equipment=[dict(zip(('unit','modification'),struct.unpack_from('<2i',data,54072+i*8))) for i in range(n)]
        owned={s['id'] for s in stocks if s['type']==17}
        if any(e['unit']<0 or e['modification']<=0 or e['modification'] not in owned for e in equipment) or len({e['unit'] for e in equipment})!=n:raise ValueError('invalid modification equipment')
    shop=dict(claims=0,allowed=[0]*3,cursors=[0]*3,games=0,pending=False,draws=[],grants=[])
    if version>=14:
        claims,*values=struct.unpack_from('<7Q4I',data,54592)
        allowed=values[:3];cursors=values[3:6];games,pending,nd,ng=values[6:]
        if games>3 or pending not in (0,1) or (pending and games) or nd>16 or ng>128 or any(c>a for a,c in zip(allowed,cursors)):raise ValueError('invalid shop journal header')
        draws=[dict(zip(('item','amount'),struct.unpack_from('<2i',data,54664+i*8))) for i in range(nd)]
        grants=[dict(zip(('type','id','amount'),struct.unpack_from('<3i',data,54792+i*12))) for i in range(ng)]
        if (not claims and any([games,pending,nd,ng])) or (claims and (nd<2 or nd>3 or not ng)):raise ValueError('invalid shop receipt')
        if any(not 12000<=r['item']<=12031 or r['amount']<=0 for r in draws) or len({r['item'] for r in draws})!=nd:raise ValueError('invalid shop draw')
        if any(r['type'] not in (0,1,2,4,7,8,9,10,11,15,17) or r['id']<0 or r['amount']<=0 or (r['type']<=2 and r['id']) or (r['type']==15 and r['id']!=1) for r in grants):raise ValueError('invalid shop grant')
        shop=dict(claims=claims,allowed=allowed,cursors=cursors,games=games,pending=bool(pending),draws=draws,grants=grants)
    contract_boxes=dict(initialized=False,baseline=0,tracks=[],claims=0,pending=False,draws=[],grants=[])
    if version>=15:
        initialized,baseline=struct.unpack_from('<Ii',data,56328)
        tracks=[dict(zip(('spent','group','step','reserved','cycles'),struct.unpack_from('<iiIIQ',data,56336+i*24))) for i in range(2)]
        claims,last,pending,nd,ng=struct.unpack_from('<Qi3I',data,56384)
        if initialized not in (0,1) or not 0<=baseline<=contract_points or pending not in (0,1) or nd>16 or ng>128:raise ValueError('invalid contract box header')
        if any(t['spent']<0 or t['spent']>contract_points-baseline or t['group']<0 or t['step']>(1 if i else 4) or (t['group']==0)!=(t['step']==0) or t['reserved'] or t['cycles']>claims for i,t in enumerate(tracks)):raise ValueError('invalid contract track')
        if not initialized and any(data[56328:58072]):raise ValueError('uninitialized contract journal has data')
        if (not claims and any([last,pending,nd,ng])) or (claims and (last<=0 or not nd or not ng)):raise ValueError('invalid contract box receipt')
        draws=[dict(zip(('item','amount'),struct.unpack_from('<2i',data,56408+i*8))) for i in range(nd)]
        grants=[dict(zip(('type','id','amount'),struct.unpack_from('<3i',data,56536+i*12))) for i in range(ng)]
        if any(r['item']<=0 or r['amount']<=0 for r in draws) or len({r['item'] for r in draws})!=nd:raise ValueError('invalid contract draw')
        if any(r['type'] not in (0,1,2,4,7,8,9,10,11,17) or r['id']<0 or r['amount']<=0 or (r['type']<=2 and r['id']) for r in grants):raise ValueError('invalid contract grant')
        contract_boxes=dict(initialized=bool(initialized),baseline=baseline,tracks=tracks,claims=claims,last_box=last,pending=bool(pending),draws=draws,grants=grants)
    competition_tracks=dict(claims=0,pending=False,tracks=[],grants=[])
    if version>=16:
        n,pending,row,premium,ng,reserved,claims=struct.unpack_from('<6IQ',data,58072)
        if n>16 or pending>n or row>=20 or premium>1 or ng>128 or reserved:raise ValueError('invalid competition track header')
        tracks=[]
        for i in range(n):
            identity,definition,points,padding,cursor,started,free,vip=struct.unpack_from('<4i2Q2I',data,58104+i*40)
            if identity<=0 or definition<=0 or points<0 or padding or (started and started<=cursor) or free>>20 or vip>>20:raise ValueError('invalid competition track')
            tracks.append(dict(competition=identity,definition=definition,points=points,cursor=cursor,started=started,taken=[free,vip]))
        if len({x['competition'] for x in tracks})!=n or sum(x.bit_count() for t in tracks for x in t['taken'])!=claims:raise ValueError('invalid competition receipts')
        if not pending and any([row,premium,ng]):raise ValueError('competition pending data without receipt')
        if pending and not tracks[pending-1]['taken'][premium]&(1<<row):raise ValueError('competition pending cell not taken')
        grants=[dict(zip(('type','id','amount'),struct.unpack_from('<3i',data,58744+i*12))) for i in range(ng)]
        if any(g['type'] not in (0,1,2,4,7,8,9,10,17) or g['id']<0 or g['amount']<=0 or (g['type']<=2 and g['id']) for g in grants):raise ValueError('invalid competition grant')
        competition_tracks=dict(claims=claims,pending=bool(pending),pending_track=pending,pending_row=row,pending_premium=premium,tracks=tracks,grants=grants)
    return dict(path=str(path), version=version, checksum_valid=True, generation=generation,
                next_run=next_run, settled_run=settled_run, credits=credits, gold=gold,
                experience=xp, stars=stars, missions=missions, upgrades=upgrades,
                career_receipts=receipts, inventory=stocks,skill_nodes=skill_nodes,quest_receipts=quest_ids,quest_warpoints=quest_warpoints,
                contract_receipts=contracts,contract_points=contract_points,contract_pvp_cursor=contract_pvp_cursor,contract_pvp_wins=contract_pvp_wins,
                warpoint_pvp_cursor=warpoint_pvp_cursor,supply=supply,shop=shop,contract_boxes=contract_boxes,competition_tracks=competition_tracks,modification_equipment=equipment,
                cooperative_credit_cursor=struct.unpack_from('<Q',data,54584)[0] if version>=13 else 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', type=Path, nargs='+')
    args = parser.parse_args()
    results = []
    for path in args.files:
        try:
            results.append(decode(path))
        except (OSError, ValueError) as error:
            results.append(dict(path=str(path), error=str(error)))
    print(json.dumps(results, ensure_ascii=False, indent=2))
    if not any(item.get('checksum_valid') for item in results):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
